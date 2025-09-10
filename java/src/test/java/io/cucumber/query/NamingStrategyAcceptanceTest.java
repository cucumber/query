package io.cucumber.query;

import io.cucumber.messages.NdjsonToMessageIterable;
import io.cucumber.messages.types.Envelope;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import static io.cucumber.query.Jackson.OBJECT_MAPPER;
import static io.cucumber.query.NamingStrategy.ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED;
import static io.cucumber.query.NamingStrategy.ExampleName.PICKLE;
import static io.cucumber.query.NamingStrategy.FeatureName.EXCLUDE;
import static io.cucumber.query.NamingStrategy.Strategy.LONG;
import static io.cucumber.query.NamingStrategy.Strategy.SHORT;
import static java.nio.charset.StandardCharsets.UTF_8;
import static java.nio.file.Files.newOutputStream;
import static java.nio.file.Files.readAllBytes;
import static org.assertj.core.api.Assertions.assertThat;

public class NamingStrategyAcceptanceTest {
    private static final NdjsonToMessageIterable.Deserializer deserializer = (json) -> OBJECT_MAPPER.readValue(json, Envelope.class);

    static List<TestCase> acceptance() {
        Map<String, NamingStrategy> strategies = new LinkedHashMap<>();
        strategies.put("long", NamingStrategy.strategy(LONG).build());
        strategies.put("long-exclude-feature-name", NamingStrategy.strategy(LONG).featureName(EXCLUDE).build());
        strategies.put("long-with-pickle-name", NamingStrategy.strategy(LONG).exampleName(PICKLE).build());
        strategies.put("long-with-pickle-name-if-parameterized", NamingStrategy.strategy(LONG).exampleName(NUMBER_AND_PICKLE_IF_PARAMETERIZED).build());
        strategies.put("short", NamingStrategy.strategy(SHORT).build());
                
        List<Path> sources = Arrays.asList(
                        Paths.get("../testdata/src/minimal.ndjson"),
                        Paths.get("../testdata/src/rules.ndjson"),
                        Paths.get("../testdata/src/examples-tables.ndjson")
                );
        
        List<TestCase> testCases = new ArrayList<>();
        sources.forEach(path ->
                strategies.forEach((strategyName, strategy) -> 
                        testCases.add(new TestCase(path, strategyName, strategy))));
        
        return testCases;
    }
    
    private static String writeResults(TestCase testCase, NamingStrategy strategy) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        writeResults(strategy, testCase, out);
        return new String(out.toByteArray(), UTF_8);
    }
    
    private static void writeResults(NamingStrategy strategy, TestCase testCase, OutputStream out) throws IOException {
        try (InputStream in = Files.newInputStream(testCase.source)) {
            try (NdjsonToMessageIterable envelopes = new NdjsonToMessageIterable(in, deserializer)) {
                try (PrintWriter writer = new PrintWriter(new BufferedWriter(new OutputStreamWriter(out)))) {
                    Query query = new Query();
                    for (Envelope envelope : envelopes) {
                        query.update(envelope);
                    }
                    query.findAllPickles().forEach(pickle -> {
                        query.findLineageBy(pickle)
                                .map(lineage -> strategy.reduce(lineage, pickle))
                                .ifPresent(writer::println);
                    });
                }

            }
        }
    }

    @ParameterizedTest
    @MethodSource("acceptance")
    void test(TestCase testCase) throws IOException {
        String actual = writeResults(testCase, testCase.strategy);
        String expected = new String(readAllBytes(testCase.expected), UTF_8);
        assertThat(actual).isEqualTo(expected);
    }

    @ParameterizedTest
    @MethodSource("acceptance")
    @Disabled
    void updateExpectedQueryResultFiles(TestCase testCase) throws IOException {
        try (OutputStream out = newOutputStream(testCase.expected)) {
            writeResults(testCase.strategy, testCase, out);
        }
    }

    static class TestCase {
        private final Path source;
        private final NamingStrategy strategy;
        private final Path expected;

        private final String name;
        private final String strategyName;

        TestCase(Path source, String strategyName, NamingStrategy strategy) {
            this.source = source;
            this.strategy = strategy;
            this.strategyName = strategyName;
            String fileName = source.getFileName().toString();
            this.name = fileName.substring(0, fileName.lastIndexOf(".ndjson"));
            this.expected = source.getParent().resolve(name  + ".naming-strategy." + strategyName + ".txt");
        }

        @Override
        public String toString() {
            return name + " -> " + strategyName;
        }

    }

}
