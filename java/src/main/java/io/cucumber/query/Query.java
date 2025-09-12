package io.cucumber.query;

import io.cucumber.messages.Convertor;
import io.cucumber.messages.TestStepResultStatusComparator;
import io.cucumber.messages.types.Attachment;
import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Hook;
import io.cucumber.messages.types.Location;
import io.cucumber.messages.types.Meta;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.PickleStep;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.Step;
import io.cucumber.messages.types.StepDefinition;
import io.cucumber.messages.types.Suggestion;
import io.cucumber.messages.types.TableRow;
import io.cucumber.messages.types.TestCase;
import io.cucumber.messages.types.TestCaseFinished;
import io.cucumber.messages.types.TestCaseStarted;
import io.cucumber.messages.types.TestRunFinished;
import io.cucumber.messages.types.TestRunStarted;
import io.cucumber.messages.types.TestStep;
import io.cucumber.messages.types.TestStepFinished;
import io.cucumber.messages.types.TestStepResult;
import io.cucumber.messages.types.TestStepResultStatus;
import io.cucumber.messages.types.TestStepStarted;
import io.cucumber.messages.types.Timestamp;

import java.time.Duration;
import java.util.AbstractMap.SimpleEntry;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.Objects;

import static java.util.Collections.emptyList;
import static java.util.Comparator.comparing;
import static java.util.Objects.requireNonNull;
import static java.util.Optional.ofNullable;
import static java.util.function.Function.identity;
import static java.util.stream.Collectors.counting;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.toList;

/**
 * Given one Cucumber Message, find another.
 * <p>
 * Queries can be made while the test run is incomplete - and this will
 * naturally return incomplete results.
 *
 * @see <a href="https://github.com/cucumber/messages?tab=readme-ov-file#message-overview">Cucumber Messages - Message Overview</a>
 */
public final class Query {
    private final Repository repository;

    public Query(Repository repository) {
        this.repository = repository;
    }

    public Map<TestStepResultStatus, Long> countMostSevereTestStepResultStatus() {
        EnumMap<TestStepResultStatus, Long> results = new EnumMap<>(TestStepResultStatus.class);
        for (TestStepResultStatus value : TestStepResultStatus.values()) {
            results.put(value, 0L);
        }
        results.putAll(findAllTestCaseStarted().stream()
                .map(this::findMostSevereTestStepResultBy)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .map(TestStepResult::getStatus)
                .collect(groupingBy(identity(), LinkedHashMap::new, counting())));
        return results;
    }
    public int countTestCasesStarted() {
        return findAllTestCaseStarted().size();
    }

    public List<Pickle> findAllPickles() {
        return new ArrayList<>(repository.pickleById.values());
    }

    public List<PickleStep> findAllPickleSteps() {
        return new ArrayList<>(repository.pickleStepById.values());
    }

    public List<TestCaseStarted> findAllTestCaseStarted() {
        return repository.testCaseStartedById.values().stream()
                .filter(element -> !findTestCaseFinishedBy(element)
                        .filter(TestCaseFinished::getWillBeRetried)
                        .isPresent())
                .collect(toList());
    }

    public List<TestCaseFinished> findAllTestCaseFinished() {
        return repository.testCaseFinishedByTestCaseStartedId.values().stream()
                .filter(testCaseFinished -> !testCaseFinished.getWillBeRetried())
                .collect(toList());
    }

    public List<TestStep> findAllTestSteps() {
        return new ArrayList<>(repository.testStepById.values());
    }

    public List<TestCase> findAllTestCases() {
        return new ArrayList<>(repository.testCaseById.values());
    }

    public List<TestStepStarted> findAllTestStepStarted() {
        return repository.testStepsStartedByTestCaseStartedId.values().stream()
                .flatMap(Collection::stream)
                .collect(toList());
    }

    public List<TestStepFinished> findAllTestStepFinished() {
        return repository.testStepsFinishedByTestCaseStartedId.values().stream()
                .flatMap(Collection::stream)
                .collect(toList());
    }

    public List<Attachment> findAttachmentsBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return repository.attachmentsByTestCaseStartedId.getOrDefault(testStepFinished.getTestCaseStartedId(), emptyList()).stream()
                .filter(attachment -> attachment.getTestStepId()
                        .map(testStepId -> testStepFinished.getTestStepId().equals(testStepId))
                        .orElse(false))
                .collect(toList());
    }

    public Optional<Hook> findHookBy(TestStep testStep) {
        requireNonNull(testStep);
        return testStep.getHookId()
                .map(repository.hookById::get);
    }

    public Optional<Meta> findMeta() {
        return ofNullable(repository.meta);
    }

    public Optional<TestStepResult> findMostSevereTestStepResultBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return findTestStepsFinishedBy(testCaseStarted)
                .stream()
                .map(TestStepFinished::getTestStepResult)
                .max(comparing(TestStepResult::getStatus, new TestStepResultStatusComparator()));
    }

    public Optional<TestStepResult> findMostSevereTestStepResultBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        return findTestCaseStartedBy(testCaseFinished)
                .flatMap(this::findMostSevereTestStepResultBy);
    }

    public Optional<Location> findLocationOf(Pickle pickle) {
        return findLineageBy(pickle).flatMap(lineage -> {
            if (lineage.example().isPresent()) {
                return lineage.example().map(TableRow::getLocation);
            }
            return lineage.scenario().map(Scenario::getLocation);
        });
    }

    public Optional<Pickle> findPickleBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return findTestCaseBy(testCaseStarted)
                .flatMap(this::findPickleBy);
    }

    public Optional<Pickle> findPickleBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        return findTestCaseStartedBy(testCaseFinished)
                .flatMap(this::findPickleBy);
    }

    public Optional<Pickle> findPickleBy(TestCase testCase) {
        requireNonNull(testCase);
        return ofNullable(repository.pickleById.get(testCase.getPickleId()));
    }

    public Optional<Pickle> findPickleBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return findTestCaseBy(testStepStarted)
                .map(TestCase::getPickleId)
                .map(repository.pickleById::get);
    }

    public Optional<Pickle> findPickleBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return findTestCaseBy(testStepFinished)
                .map(TestCase::getPickleId)
                .map(repository.pickleById::get);
    }

    public Optional<PickleStep> findPickleStepBy(TestStep testStep) {
        requireNonNull(testStep);
        return testStep.getPickleStepId()
                .map(repository.pickleStepById::get);
    }

    public List<Suggestion> findSuggestionsBy(PickleStep pickleStep) {
        requireNonNull(pickleStep);
        List<Suggestion> suggestions = repository.suggestionsByPickleStepId.getOrDefault(pickleStep.getId(), emptyList());
        return new ArrayList<>(suggestions);
    }

    public List<Suggestion> findSuggestionsBy(Pickle pickle) {
        requireNonNull(pickle);
        return pickle.getSteps().stream()
                .map(this::findSuggestionsBy)
                .flatMap(Collection::stream)
                .collect(toList());
    }

    public Optional<Step> findStepBy(PickleStep pickleStep) {
        requireNonNull(pickleStep);
        String stepId = pickleStep.getAstNodeIds().get(0);
        return ofNullable(repository.stepById.get(stepId));
    }

    public List<StepDefinition> findStepDefinitionsBy(TestStep testStep) {
        requireNonNull(testStep);
        return testStep.getStepDefinitionIds().map(ids -> ids.stream()
                        .map(repository.stepDefinitionById::get)
                        .filter(Objects::nonNull)
                        .collect(toList()))
                .orElseGet(Collections::emptyList);
    }

    public Optional<StepDefinition> findUnambiguousStepDefinitionBy(TestStep testStep) {
        requireNonNull(testStep);
        return testStep.getStepDefinitionIds()
                .filter(ids -> ids.size() == 1)
                .map(ids -> repository.stepDefinitionById.get(ids.get(0)));
    }

    public Optional<TestCase> findTestCaseBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return ofNullable(repository.testCaseById.get(testCaseStarted.getTestCaseId()));
    }

    public Optional<TestCase> findTestCaseBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        return findTestCaseStartedBy(testCaseFinished)
                .flatMap(this::findTestCaseBy);
    }

    public Optional<TestCase> findTestCaseBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return findTestCaseStartedBy(testStepStarted)
                .flatMap(this::findTestCaseBy);
    }

    public Optional<TestCase> findTestCaseBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return findTestCaseStartedBy(testStepFinished)
                .flatMap(this::findTestCaseBy);
    }

    public Optional<Duration> findTestCaseDurationBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        Timestamp started = testCaseStarted.getTimestamp();
        return findTestCaseFinishedBy(testCaseStarted)
                .map(TestCaseFinished::getTimestamp)
                .map(finished -> Duration.between(
                        Convertor.toInstant(started),
                        Convertor.toInstant(finished)
                ));
    }

    public Optional<Duration> findTestCaseDurationBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        return findTestCaseStartedBy(testCaseFinished)
                .flatMap(this::findTestCaseDurationBy);
    }

    public Optional<TestCaseStarted> findTestCaseStartedBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        String testCaseStartedId = testStepStarted.getTestCaseStartedId();
        return ofNullable(repository.testCaseStartedById.get(testCaseStartedId));
    }

    public Optional<TestCaseStarted> findTestCaseStartedBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        String testCaseStartedId = testCaseFinished.getTestCaseStartedId();
        return ofNullable(repository.testCaseStartedById.get(testCaseStartedId));
    }

    public Optional<TestCaseStarted> findTestCaseStartedBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        String testCaseStartedId = testStepFinished.getTestCaseStartedId();
        return ofNullable(repository.testCaseStartedById.get(testCaseStartedId));
    }

    public Optional<TestCaseFinished> findTestCaseFinishedBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return ofNullable(repository.testCaseFinishedByTestCaseStartedId.get(testCaseStarted.getId()));
    }

    public Optional<Duration> findTestRunDuration() {
        if (repository.testRunStarted == null || repository.testRunFinished == null) {
            return Optional.empty();
        }
        Duration between = Duration.between(
                Convertor.toInstant(repository.testRunStarted.getTimestamp()),
                Convertor.toInstant(repository.testRunFinished.getTimestamp())
        );
        return Optional.of(between);
    }

    public Optional<TestRunFinished> findTestRunFinished() {
        return ofNullable(repository.testRunFinished);
    }

    public Optional<TestRunStarted> findTestRunStarted() {
        return ofNullable(repository.testRunStarted);
    }

    public Optional<TestStep> findTestStepBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return ofNullable(repository.testStepById.get(testStepStarted.getTestStepId()));
    }

    public Optional<TestStep> findTestStepBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return ofNullable(repository.testStepById.get(testStepFinished.getTestStepId()));
    }

    public List<TestStepStarted> findTestStepsStartedBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        List<TestStepStarted> testStepsStarted = repository.testStepsStartedByTestCaseStartedId.
                getOrDefault(testCaseStarted.getId(), emptyList());
        // Concurrency
        return new ArrayList<>(testStepsStarted);
    }

    public List<TestStepStarted> findTestStepsStartedBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        List<TestStepStarted> testStepsStarted = repository.testStepsStartedByTestCaseStartedId.
                getOrDefault(testCaseFinished.getTestCaseStartedId(), emptyList());
        // Concurrency
        return new ArrayList<>(testStepsStarted);
    }

    public List<TestStepFinished> findTestStepsFinishedBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        List<TestStepFinished> testStepsFinished = repository.testStepsFinishedByTestCaseStartedId.
                getOrDefault(testCaseStarted.getId(), emptyList());
        // Concurrency
        return new ArrayList<>(testStepsFinished);
    }

    public List<TestStepFinished> findTestStepsFinishedBy(TestCaseFinished testCaseFinished) {
        requireNonNull(testCaseFinished);
        return findTestCaseStartedBy(testCaseFinished)
                .map(this::findTestStepsFinishedBy)
                .orElseGet(ArrayList::new);
    }

    public List<Entry<TestStepFinished, TestStep>> findTestStepFinishedAndTestStepBy(TestCaseStarted testCaseStarted) {
        return findTestStepsFinishedBy(testCaseStarted).stream()
                .map(testStepFinished -> findTestStepBy(testStepFinished).map(testStep -> new SimpleEntry<>(testStepFinished, testStep)))
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(toList());
    }

    public Optional<Lineage> findLineageBy(GherkinDocument element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element.getUri()));
    }

    public Optional<Lineage> findLineageBy(Feature element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element));
    }

    public Optional<Lineage> findLineageBy(Rule element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Scenario element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Examples element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(TableRow element) {
        requireNonNull(element);
        return Optional.ofNullable(repository.lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Pickle pickle) {
        requireNonNull(pickle);
        List<String> astNodeIds = pickle.getAstNodeIds();
        String pickleAstNodeId = astNodeIds.get(astNodeIds.size() - 1);
        return Optional.ofNullable(repository.lineageById.get(pickleAstNodeId));
    }

    public Optional<Lineage> findLineageBy(TestCaseStarted testCaseStarted) {
        return findPickleBy(testCaseStarted)
                .flatMap(this::findLineageBy);
    }
}
