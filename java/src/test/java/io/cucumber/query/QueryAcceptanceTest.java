package io.cucumber.query;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import io.cucumber.messages.Convertor;
import io.cucumber.messages.NdjsonToMessageIterable;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.Hook;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.PickleStep;
import io.cucumber.messages.types.Step;
import io.cucumber.messages.types.TestCaseFinished;
import io.cucumber.messages.types.TestCaseStarted;
import io.cucumber.messages.types.TestStep;
import io.cucumber.messages.types.TestStepFinished;
import io.cucumber.messages.types.TestStepResult;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Stream;

import static com.fasterxml.jackson.core.util.DefaultIndenter.SYSTEM_LINEFEED_INSTANCE;
import static io.cucumber.query.Jackson.OBJECT_MAPPER;
import static io.cucumber.query.NamingStrategy.ExampleName.PICKLE;
import static io.cucumber.query.NamingStrategy.FeatureName.EXCLUDE;
import static io.cucumber.query.NamingStrategy.Strategy.LONG;
import static io.cucumber.query.NamingStrategy.Strategy.SHORT;
import static java.nio.charset.StandardCharsets.UTF_8;
import static java.util.Arrays.asList;
import static java.util.stream.Collectors.toList;
import static org.assertj.core.api.Assertions.assertThat;

public class QueryAcceptanceTest {
    private static final NdjsonToMessageIterable.Deserializer deserializer = (json) -> OBJECT_MAPPER.readValue(json, Envelope.class);

    static List<TestCase> acceptance() {

        return Stream.of(
                        Paths.get("../testdata/attachments.feature.ndjson"),
                        Paths.get("../testdata/empty.feature.ndjson"),
                        Paths.get("../testdata/hooks.feature.ndjson"),
                        Paths.get("../testdata/minimal.feature.ndjson"),
                        Paths.get("../testdata/rules.feature.ndjson"),
                        Paths.get("../testdata/examples-tables.feature.ndjson")
                )
                .map(TestCase::new)
                .sorted(Comparator.comparing(testCase -> testCase.source))
                .collect(toList());
    }

    @ParameterizedTest
    @MethodSource("acceptance")
    void test(TestCase testCase) throws IOException {
        ByteArrayOutputStream bytes = writeQueryResults(testCase, new ByteArrayOutputStream());
        String expected = new String(Files.readAllBytes(testCase.expected), UTF_8);
        String actual = new String(bytes.toByteArray(), UTF_8);
        assertThat(actual).isEqualTo(expected);
    }


    @ParameterizedTest
    @MethodSource("acceptance")
    @Disabled
    void updateExpectedQueryResultFiles(TestCase testCase) throws IOException {
        try (OutputStream out = Files.newOutputStream(testCase.expected)) {
            writeQueryResults(testCase, out);
        }
    }

    private static <T extends OutputStream> T writeQueryResults(TestCase testCase, T out) throws IOException {
        try (InputStream in = Files.newInputStream(testCase.source)) {
            try (NdjsonToMessageIterable envelopes = new NdjsonToMessageIterable(in, deserializer)) {
                Query query = new Query();
                for (Envelope envelope : envelopes) {
                    query.update(envelope);
                }
                Map<String, Object> queryResults = createQueryResults(query);
                DefaultPrettyPrinter prettyPrinter = new DefaultPrettyPrinter()
                        .withArrayIndenter(SYSTEM_LINEFEED_INSTANCE);
                OBJECT_MAPPER.writer(prettyPrinter).writeValue(out, queryResults);
            }
        }
        return out;
    }

    private static Map<String, Object> createQueryResults(Query query) {

        Map<String, Object> results = new LinkedHashMap<>();

        results.put("countMostSevereTestStepResultStatus", query.countMostSevereTestStepResultStatus());
        results.put("countTestCasesStarted", query.countTestCasesStarted());
        results.put("findAllPickles", query.findAllPickles().size());
        results.put("findAllPickleSteps", query.findAllPickleSteps().size());
        results.put("findAllTestCaseStarted", query.findAllTestCaseStarted().size());
        results.put("findAllTestSteps", query.findAllTestSteps().size());
        results.put("findAllTestCaseStartedGroupedByFeature", query.findAllTestCaseStartedGroupedByFeature()
                .entrySet()
                .stream()
                .map(entry -> Arrays.asList(entry.getKey().map(Feature::getName), entry.getValue().stream()
                        .map(TestCaseStarted::getId)
                        .collect(toList()))));
        results.put("findAttachmentsBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestStepFinishedAndTestStepBy)
                .flatMap(Collection::stream)
                .map(Map.Entry::getKey)
                .map(query::findAttachmentsBy)
                .flatMap(Collection::stream)
                        .map(attachment -> Arrays.asList(
                                attachment.getTestStepId(),
                                attachment.getTestCaseStartedId(),
                                attachment.getMediaType(),
                                attachment.getContentEncoding()
                        ))
                .collect(toList()));
        results.put("findFeatureBy", query.findAllTestCaseStarted().stream()
                .map(query::findFeatureBy)
                .map(feature -> feature.map(Feature::getName))
                .collect(toList()));
        results.put("findHookBy", query.findAllTestSteps().stream()
                .map(query::findHookBy)
                .map(hook -> hook.map(Hook::getId))
                .filter(Optional::isPresent)
                .collect(toList()));
        results.put("findMostSevereTestStepResultBy", query.findAllTestCaseStarted().stream()
                .map(query::findMostSevereTestStepResultBy)
                .map(testStepResult -> testStepResult.map(TestStepResult::getStatus))
                .collect(toList()));

        Map<String, Object> names = new LinkedHashMap<>();
        names.put("long", query.findAllPickles().stream()
                .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).build()))
                .collect(toList()));
        names.put("excludeFeatureName", query.findAllPickles().stream()
                .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).featureName(EXCLUDE).build()))
                .collect(toList()));
        names.put("longPickleName", query.findAllPickles().stream()
                .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).exampleName(PICKLE).build()))
                .collect(toList()));
        names.put("short", query.findAllPickles().stream()
                .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(SHORT).build()))
                .collect(toList()));
        names.put("shortPickleName", query.findAllPickles().stream()
                .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(SHORT).exampleName(PICKLE).build()))
                .collect(toList()));

        results.put("findNameOf", names);

        results.put("findPickleBy", query.findAllTestCaseStarted().stream()
                .map(query::findPickleBy)
                .map(pickle -> pickle.map(Pickle::getName))
                .collect(toList()));
        results.put("findPickleStepBy", query.findAllTestSteps().stream()
                .map(query::findPickleStepBy)
                .map(pickleStep -> pickleStep.map(PickleStep::getText))
                .filter(Optional::isPresent)
                .collect(toList()));
        results.put("findStepBy", query.findAllPickleSteps().stream()
                .map(query::findStepBy)
                .map(step -> step.map(Step::getText))
                .collect(toList()));
        results.put("findTestCaseBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestCaseBy)
                .map(testCase -> testCase.map(io.cucumber.messages.types.TestCase::getId))
                .collect(toList()));
        results.put("findTestCaseDurationBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestCaseDurationBy)
                .map(duration -> duration.map(Convertor::toMessage))
                .collect(toList()));
        results.put("findTestCaseFinishedBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestCaseFinishedBy)
                .map(testCaseFinished -> testCaseFinished.map(TestCaseFinished::getTestCaseStartedId))
                .collect(toList()));
        results.put("findTestRunDuration", query.findTestRunDuration()
                .map(Convertor::toMessage));
        results.put("findTestRunFinished", query.findTestRunFinished());
        results.put("findTestRunStarted", query.findTestRunStarted());
        results.put("findTestStepBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestStepsFinishedBy)
                .flatMap(Collection::stream)
                .map(query::findTestStepBy)
                .map(testStep -> testStep.map(TestStep::getId))
                .collect(toList()));
        results.put("findTestStepsFinishedBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestStepsFinishedBy)
                .map(testStepFinisheds -> testStepFinisheds.stream().map(TestStepFinished::getTestStepId).collect(toList()))
                .collect(toList()));
        results.put("findTestStepFinishedAndTestStepBy", query.findAllTestCaseStarted().stream()
                .map(query::findTestStepFinishedAndTestStepBy)
                .flatMap(Collection::stream)
                .map(entry -> asList(entry.getKey().getTestStepId(), entry.getValue().getId()))
                .collect(toList()));

        return results;
    }

    static class TestCase {
        private final Path source;
        private final Path expected;

        private final String name;

        TestCase(Path source) {
            this.source = source;
            String fileName = source.getFileName().toString();
            this.name = fileName.substring(0, fileName.lastIndexOf(".ndjson"));
            this.expected = source.getParent().resolve(name + ".query-results.json");
        }

        @Override
        public String toString() {
            return name;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            TestCase testCase = (TestCase) o;
            return source.equals(testCase.source);
        }

        @Override
        public int hashCode() {
            return Objects.hash(source);
        }
    }

}
