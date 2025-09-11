package io.cucumber.query;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import io.cucumber.messages.Convertor;
import io.cucumber.messages.NdjsonToMessageIterable;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.Hook;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.PickleStep;
import io.cucumber.messages.types.Step;
import io.cucumber.messages.types.StepDefinition;
import io.cucumber.messages.types.Suggestion;
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
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_ATTACHMENTS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_GHERKIN_DOCUMENT;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_HOOKS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_STEP_DEFINITIONS;
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
                Paths.get("../testdata/src/attachments.ndjson"),
                Paths.get("../testdata/src/empty.ndjson"),
                Paths.get("../testdata/src/hooks.ndjson"),
                Paths.get("../testdata/src/minimal.ndjson"),
                Paths.get("../testdata/src/rules.ndjson"),
                Paths.get("../testdata/src/examples-tables.ndjson")
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
                Repository repository = createRepository();
                for (Envelope envelope : envelopes) {
                    repository.update(envelope);
                }
                Query query = new Query(repository);
                Object queryResults = testCase.query.apply(query);
                DefaultPrettyPrinter prettyPrinter = new DefaultPrettyPrinter()
                        .withArrayIndenter(SYSTEM_LINEFEED_INSTANCE);
                OBJECT_MAPPER.writer(prettyPrinter).writeValue(out, queryResults);
            }
        }
        return out;
    }

    private static Repository createRepository() {
        return Repository.builder()
                .feature(INCLUDE_ATTACHMENTS, true)
                .feature(INCLUDE_STEP_DEFINITIONS, true)
                .feature(INCLUDE_HOOKS, true)
                .feature(INCLUDE_GHERKIN_DOCUMENT, true)
                .build();
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
        queries.put("findAllTestStepsStarted", (query) -> query.findAllTestStepStarted().size());
        queries.put("findAllTestStepsFinished", (query) -> query.findAllTestStepFinished().size());
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
        queries.put("findHookBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findHookBy)
                .map(hook -> hook.map(Hook::getId))
                .filter(Optional::isPresent)
                .collect(toList()));

        queries.put("findLineageBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            NamingStrategy namingStrategy = NamingStrategy.strategy(NamingStrategy.Strategy.LONG).build();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findLineageBy)
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .map(namingStrategy::reduce)
                    .collect(toList()));
            results.put("pickle", query.findAllPickles().stream()
                    .map(query::findLineageBy)
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .map(namingStrategy::reduce)
                    .collect(toList()));
            return results;
        });
        
        queries.put("findLocationOf", (query) -> query.findAllPickles().stream()
                .map(query::findLocationOf)
                .filter(Optional::isPresent)
                .collect(toList()));
        queries.put("findMeta", (query) -> query.findMeta().map(meta -> meta.getImplementation().getName()));

        queries.put("findMostSevereTestStepResultBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findMostSevereTestStepResultBy)
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .map(TestStepResult::getStatus)
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findMostSevereTestStepResultBy)
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .map(TestStepResult::getStatus)
                    .collect(toList()));
            return results;
        });

        queries.put("findPickleBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findPickleBy)
                    .map(pickle -> pickle.map(Pickle::getName))
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findPickleBy)
                    .map(pickle -> pickle.map(Pickle::getName))
                    .collect(toList()));
            results.put("testStepStarted", query.findAllTestStepStarted().stream()
                    .map(query::findPickleBy)
                    .map(pickle -> pickle.map(Pickle::getName))
                    .collect(toList()));
            results.put("testStepFinished", query.findAllTestStepFinished().stream()
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

        queries.put("findSuggestionsBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("pickleStep", query.findAllPickleSteps().stream()
                    .map(query::findSuggestionsBy)
                    .flatMap(Collection::stream)
                    .map(Suggestion::getId)
                    .collect(toList()));
            results.put("pickle", query.findAllPickles().stream()
                    .map(query::findSuggestionsBy)
                    .flatMap(Collection::stream)
                    .map(Suggestion::getId)
                    .collect(toList()));
            return results;
        });

        queries.put("findUnambiguousStepDefinitionBy", (query) -> query.findAllTestSteps().stream()
                .map(query::findUnambiguousStepDefinitionBy)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .map(StepDefinition::getId));

        queries.put("findTestCaseStartedBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testStepStarted", query.findAllTestStepStarted().stream()
                    .map(query::findTestCaseStartedBy)
                    .map(testCase -> testCase.map(TestCaseStarted::getId))
                    .collect(toList()));
            results.put("testStepFinished", query.findAllTestStepFinished().stream()
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
            results.put("testStepStarted", query.findAllTestStepStarted().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            results.put("testStepFinished", query.findAllTestStepFinished().stream()
                    .map(query::findTestCaseBy)
                    .map(testCase -> testCase.map(TestCase::getId))
                    .collect(toList()));
            return results;
        });

        queries.put("findTestCaseDurationBy", (query) -> {
            Map<String, Object> results = new LinkedHashMap<>();
            results.put("testCaseStarted", query.findAllTestCaseStarted().stream()
                    .map(query::findTestCaseDurationBy)
                    .map(duration -> duration.map(Convertor::toMessage))
                    .collect(toList()));
            results.put("testCaseFinished", query.findAllTestCaseFinished().stream()
                    .map(query::findTestCaseDurationBy)
                    .map(duration -> duration.map(Convertor::toMessage))
                    .collect(toList()));
            return results;
        });

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
