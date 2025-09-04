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
import io.cucumber.messages.types.StepDefinition;
import io.cucumber.messages.types.TestCase;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;

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

    static List<QueryTestCase> acceptance() {
        List<QueryTestCase> testCases = new ArrayList<>();

        List<Path> sources = getSources();
        Map<String, Function<Query, Object>> queries = createQueries();

        sources.forEach(source ->
                queries.forEach((methodName, query) ->
                        testCases.add(new QueryTestCase(methodName, source, query))));

        return testCases;
    }

    private static List<Path> getSources() {
        return Arrays.asList(
                Paths.get("../testdata/attachments.ndjson"),
                Paths.get("../testdata/empty.ndjson"),
                Paths.get("../testdata/hooks.ndjson"),
                Paths.get("../testdata/minimal.ndjson"),
                Paths.get("../testdata/rules.ndjson"),
                Paths.get("../testdata/examples-tables.ndjson")
        );
    }

    @ParameterizedTest
    @MethodSource("acceptance")
    void test(QueryTestCase testCase) throws IOException {
        ByteArrayOutputStream bytes = writeQueryResults(testCase, new ByteArrayOutputStream());
        String expected = new String(Files.readAllBytes(testCase.expected), UTF_8);
        String actual = new String(bytes.toByteArray(), UTF_8);
        assertThat(actual).isEqualTo(expected);
    }

    @ParameterizedTest
    @MethodSource("acceptance")
    @Disabled
    void updateExpectedQueryResultFiles(QueryTestCase testCase) throws IOException {
        try (OutputStream out = Files.newOutputStream(testCase.expected)) {
            writeQueryResults(testCase, out);
        }
    }

    private static <T extends OutputStream> T writeQueryResults(QueryTestCase testCase, T out) throws IOException {
        try (InputStream in = Files.newInputStream(testCase.source)) {
            try (NdjsonToMessageIterable envelopes = new NdjsonToMessageIterable(in, deserializer)) {
                Query query = new Query();
                for (Envelope envelope : envelopes) {
                    query.update(envelope);
                }
                Object queryResults = testCase.query.apply(query);
                DefaultPrettyPrinter prettyPrinter = new DefaultPrettyPrinter()
                        .withArrayIndenter(SYSTEM_LINEFEED_INSTANCE);
                OBJECT_MAPPER.writer(prettyPrinter).writeValue(out, queryResults);
            }
        }
        return out;
    }

    static Map<String, Function<Query, Object>> createQueries() {

        Map<String, Function<Query, Object>> queries = new LinkedHashMap<>();

        queries.put("countMostSevereTestStepResultStatus", Query::countMostSevereTestStepResultStatus);
        queries.put("countTestCasesStarted", Query::countTestCasesStarted);
        queries.put("findAllPickles", (query) -> query.findAllPickles().size());
        queries.put("findAllPickleSteps", (query) -> query.findAllPickleSteps().size());
        queries.put("findAllTestCaseStarted", (query) -> query.findAllTestCaseStarted().size());
        queries.put("findAllTestCaseFinished", (query) -> query.findAllTestCaseFinished().size());
        queries.put("findAllTestSteps", (query) -> query.findAllTestSteps().size());
        queries.put("findAllTestStepsStarted", (query) -> query.findAllTestStepsStarted().size());
        queries.put("findAllTestStepsFinished", (query) -> query.findAllTestStepsFinished().size());
        queries.put("findAllTestCaseStartedGroupedByFeature", (query) -> query.findAllTestCaseStartedGroupedByFeature()
                .entrySet()
                .stream()
                .map(entry -> Arrays.asList(entry.getKey().map(Feature::getName), entry.getValue().stream()
                        .map(TestCaseStarted::getId)
                        .collect(toList()))));
        queries.put("findAllTestCases", (query) -> query.findAllTestCases().size());        
        queries.put("findAttachmentsBy", (query) -> query.findAllTestCaseStarted().stream()
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
        queries.put("findFeatureBy", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findFeatureBy)
                .map(feature -> feature.map(Feature::getName))
                .collect(toList()));
        queries.put("findHookBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findHookBy)
                .map(hook -> hook.map(Hook::getId))
                .filter(Optional::isPresent)
                .collect(toList()));
        queries.put("findLocationOf", (query) -> query.findAllPickles().stream()
                .map(query::findLocationOf)
                .filter(Optional::isPresent)
                .collect(toList()));
        queries.put("findMeta", (query) -> query.findMeta().map(meta -> meta.getImplementation().getName()));
        
        queries.put("findMostSevereTestStepResultBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findMostSevereTestStepResultBy)
                    .map(testStepResult -> testStepResult.map(TestStepResult::getStatus))
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findMostSevereTestStepResultBy)
                    .map(testStepResult -> testStepResult.map(TestStepResult::getStatus))
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .collect(toList()));
            return results;
        });

        queries.put("findNameOf", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("long", query.findAllPickles().stream()
                    .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).build()))
                    .collect(toList()));
            results.put("excludeFeatureName", query.findAllPickles().stream()
                    .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).featureName(EXCLUDE).build()))
                    .collect(toList()));
            results.put("longPickleName", query.findAllPickles().stream()
                    .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(LONG).exampleName(PICKLE).build()))
                    .collect(toList()));
            results.put("short", query.findAllPickles().stream()
                    .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(SHORT).build()))
                    .collect(toList()));
            results.put("shortPickleName", query.findAllPickles().stream()
                    .map(pickle -> query.findNameOf(pickle, NamingStrategy.strategy(SHORT).exampleName(PICKLE).build()))
                    .collect(toList()));
            
            return results;
        });

        queries.put("findPickleBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted",  query.findAllTestCaseStarted().stream()
                    .map(query::findPickleBy)
                    .map(pickle -> pickle.map(Pickle::getName))
                    .collect(toList()));
            results.put("testCaseFinished",  query.findAllTestCaseFinished().stream()
                    .map(query::findPickleBy)
                    .map(pickle -> pickle.map(Pickle::getName))
                    .collect(toList()));
            return results;
        });
        
        queries.put("findPickleStepBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findPickleStepBy)
                .map(pickleStep -> pickleStep.map(PickleStep::getText))
                .filter(Optional::isPresent)
                .collect(toList()));
        queries.put("findStepBy", (query) -> query.findAllPickleSteps().stream()
                .map(query::findStepBy)
                .map(step -> step.map(Step::getText))
                .collect(toList()));
        queries.put("findStepDefinitionsBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findStepDefinitionsBy)
                .map(stepDefinitions -> stepDefinitions.stream().map(StepDefinition::getId)
                        .collect(toList()))
                .collect(toList()));
        queries.put("findUnambiguousStepDefinitionBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findUnambiguousStepDefinitionBy)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .map(StepDefinition::getId));

        queries.put("findTestCaseStartedBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testStepStarted", query.findAllTestStepsStarted().stream()
                    .map(query::findTestCaseStartedBy)
                    .map(testCase -> testCase.map(TestCaseStarted::getId))
                    .collect(toList()));
            results.put("testStepFinished", query.findAllTestStepsFinished().stream()
                    .map(query::findTestCaseStartedBy)
                    .map(testCase -> testCase.map(TestCaseStarted::getId))
                    .collect(toList()));
            return results;
        });
        
        queries.put("findTestCaseBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            results.put("testStepStarted", query.findAllTestStepsStarted().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            results.put("testStepFinished", query.findAllTestStepsFinished().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            return results;
        });
        
        queries.put("findTestCaseDurationBy", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findTestCaseDurationBy)
                .map(duration -> duration.map(Convertor::toMessage))
                .collect(toList()));
        queries.put("findTestCaseFinishedBy", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findTestCaseFinishedBy)
                .map(testCaseFinished -> testCaseFinished.map(TestCaseFinished::getTestCaseStartedId))
                .collect(toList()));
        queries.put("findTestRunDuration", (query) -> query.findTestRunDuration()
                .map(Convertor::toMessage));
        queries.put("findTestRunFinished", Query::findTestRunFinished);
        queries.put("findTestRunStarted", Query::findTestRunStarted);
        queries.put("findTestStepByTestStepStarted", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findTestStepsStartedBy)
                .flatMap(Collection::stream)
                .map(query::findTestStepBy)
                .map(testStep -> testStep.map(TestStep::getId))
                .collect(toList()));
        
        queries.put("findTestStepByTestStepFinished", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();

            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findTestStepsFinishedBy)
                    .flatMap(Collection::stream)
                    .map(query::findTestStepBy)
                    .map(testStep -> testStep.map(TestStep::getId))
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findTestStepsFinishedBy)
                    .flatMap(Collection::stream)
                    .map(query::findTestStepBy)
                    .map(testStep -> testStep.map(TestStep::getId))
                    .collect(toList()));

            return results;
        });
        
        queries.put("findTestStepsFinishedBy", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findTestStepsFinishedBy)
                .map(testStepFinisheds -> testStepFinisheds.stream().map(TestStepFinished::getTestStepId).collect(toList()))
                .collect(toList()));
        queries.put("findTestStepFinishedAndTestStepBy", (query) -> query.findAllTestCaseStarted().stream()
                .map(query::findTestStepFinishedAndTestStepBy)
                .flatMap(Collection::stream)
                .map(entry -> asList(entry.getKey().getTestStepId(), entry.getValue().getId()))
                .collect(toList()));

        return queries;
    }

    static class QueryTestCase {
        private final String methodName;
        private final String name;
        private final Path source;
        private final Path expected;
        private final Function<Query, Object> query;

        QueryTestCase(String methodName, Path source, Function<Query, Object> query) {
            this.methodName = methodName;
            this.source = source;
            this.query = query;
            String fileName = source.getFileName().toString();
            this.name = fileName.substring(0, fileName.lastIndexOf(".ndjson"));
            this.expected = source.getParent().resolve(name + "." + methodName + ".results.json");
        }

        @Override
        public String toString() {
            return name + " -> " + methodName;
        }
    }

}
