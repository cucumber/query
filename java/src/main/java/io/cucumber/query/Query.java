package io.cucumber.query;

import io.cucumber.messages.Convertor;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.PickleStep;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.Step;
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
import io.cucumber.messages.types.Timestamp;

import java.time.Duration;
import java.util.AbstractMap.SimpleEntry;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Deque;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.function.BiFunction;

import static java.util.Collections.emptyList;
import static java.util.Comparator.comparing;
import static java.util.Comparator.nullsFirst;
import static java.util.Objects.requireNonNull;
import static java.util.Optional.ofNullable;
import static java.util.function.Function.identity;
import static java.util.stream.Collectors.collectingAndThen;
import static java.util.stream.Collectors.counting;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.toList;

/**
 * Given one Cucumber Message, find another.
 * <p>
 * This class is effectively a simple in memory database. It can be updated in
 * real time through the {@link #update(Envelope)} method. Queries can be made
 * while the test run is incomplete - and this will naturally return incomplete
 * results.
 * <p>
 * It is safe to query and update this class concurrently.
 *
 * @see <a href="https://github.com/cucumber/messages?tab=readme-ov-file#message-overview">Cucumber Messages - Message Overview</a>
 *
 */
public final class Query {
    private final Comparator<TestStepResult> testStepResultComparator = nullsFirst(comparing(o -> o.getStatus().ordinal()));
    private final Deque<TestCaseStarted> testCaseStarted = new ConcurrentLinkedDeque<>();
    private final Map<String, TestCaseFinished> testCaseFinishedByTestCaseStartedId = new ConcurrentHashMap<>();
    private final Map<String, List<TestStepFinished>> testStepsFinishedByTestCaseStartedId = new ConcurrentHashMap<>();
    private final Map<String, Pickle> pickleById = new ConcurrentHashMap<>();
    private final Map<String, TestCase> testCaseById = new ConcurrentHashMap<>();
    private final Map<String, Step> stepById = new ConcurrentHashMap<>();
    private final Map<String, TestStep> testStepById = new ConcurrentHashMap<>();
    private final Map<String, PickleStep> pickleStepById = new ConcurrentHashMap<>();
    private final Map<String, GherkinDocumentElements> gherkinAstNodesById = new ConcurrentHashMap<>();
    private TestRunStarted testRunStarted;
    private TestRunFinished testRunFinished;

    public Map<TestStepResultStatus, Long> countMostSevereTestStepResultStatus() {
        return findAllTestCaseStarted().stream()
                .map(this::findMostSevereTestStepResulBy)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .map(TestStepResult::getStatus)
                .collect(groupingBy(identity(), LinkedHashMap::new, counting()));
    }

    public int countTestCasesStarted() {
        return testCaseStarted.size();
    }

    public List<Pickle> findAllPickles() {
        return pickleById.values().stream()
                .sorted(comparing(Pickle::getId))
                .collect(toList());
    }

    public List<PickleStep> findAllPickleSteps() {
        return pickleStepById.values().stream()
                .sorted(comparing(PickleStep::getId))
                .collect(toList());
    }

    public List<TestCaseStarted> findAllTestCaseStarted() {
        return new ArrayList<>(testCaseStarted);
    }

    public Map<Optional<Feature>, List<TestCaseStarted>> findAllTestCaseStartedGroupedByFeature() {
        return findAllTestCaseStarted()
                .stream()
                .map(testCaseStarted1 -> {
                    Optional<GherkinDocumentElements> astNodes = findGherkinAstNodesBy(testCaseStarted1);
                    return new SimpleEntry<>(astNodes, testCaseStarted1);
                })
                // Sort entries by gherkin document URI for consistent ordering
                .sorted(nullsFirst(comparing(entry1 -> entry1.getKey()
                        .flatMap(nodes -> nodes.document().getUri())
                        .orElse(null))))
                .map(entry -> {
                    // Unpack the now sorted entries
                    Optional<Feature> feature = entry.getKey().flatMap(GherkinDocumentElements::feature);
                    TestCaseStarted testcaseStarted = entry.getValue();
                    return new SimpleEntry<>(feature, testcaseStarted);
                })
                // Group into a linked hashmap to preserve order
                .collect(groupingBy(SimpleEntry::getKey, LinkedHashMap::new, collectingAndThen(toList(),
                        entries -> entries.stream().map(SimpleEntry::getValue)
                                .collect(toList()))));
    }

    public List<TestStep> findAllTestSteps() {
        return testStepById.values().stream()
                .sorted(comparing(TestStep::getId))
                .collect(toList());
    }

    public Optional<Feature> findFeatureBy(TestCaseStarted testCaseStarted) {
        return findGherkinAstNodesBy(testCaseStarted).flatMap(GherkinDocumentElements::feature);
    }

    public Optional<TestStepResult> findMostSevereTestStepResulBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return findTestStepsFinishedBy(testCaseStarted)
                .stream()
                .map(TestStepFinished::getTestStepResult)
                .max(testStepResultComparator);
    }

    public String findNameOf(Pickle pickle, NamingStrategy namingStrategy) {
        requireNonNull(pickle);
        requireNonNull(namingStrategy);

        return findGherkinAstNodesBy(pickle)
                .map(gherkinDocumentElements -> namingStrategy.name(gherkinDocumentElements, pickle))
                .orElse(pickle.getName());
    }

    public Optional<Pickle> findPickleBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return findTestCaseBy(testCaseStarted)
                .map(TestCase::getPickleId)
                .map(pickleById::get);
    }

    public Optional<PickleStep> findPickleStepBy(TestStep testStep) {
        requireNonNull(testCaseStarted);
        return testStep.getPickleStepId()
                .map(pickleStepById::get);
    }

    public Optional<Step> findStepBy(PickleStep pickleStep) {
        requireNonNull(pickleStep);
        String stepId = pickleStep.getAstNodeIds().get(0);
        return ofNullable(stepById.get(stepId));
    }

    public Optional<TestCase> findTestCaseBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return ofNullable(testCaseById.get(testCaseStarted.getTestCaseId()));
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

    public Optional<TestCaseFinished> findTestCaseFinishedBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return ofNullable(testCaseFinishedByTestCaseStartedId.get(testCaseStarted.getId()));
    }

    public Optional<Duration> findTestRunDuration() {
        if (testRunStarted == null || testRunFinished == null) {
            return Optional.empty();
        }
        Duration between = Duration.between(
                Convertor.toInstant(testRunStarted.getTimestamp()),
                Convertor.toInstant(testRunFinished.getTimestamp())
        );
        return Optional.of(between);
    }

    public Optional<TestRunFinished> findTestRunFinished() {
        return ofNullable(testRunFinished);
    }

    public Optional<TestRunStarted> findTestRunStarted() {
        return ofNullable(testRunStarted);
    }

    public Optional<TestStep> findTestStepBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return ofNullable(testStepById.get(testStepFinished.getTestStepId()));
    }

    public List<TestStepFinished> findTestStepsFinishedBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        List<TestStepFinished> testStepsFinished = testStepsFinishedByTestCaseStartedId.
                getOrDefault(testCaseStarted.getId(), emptyList());
        // Concurrency
        return new ArrayList<>(testStepsFinished);
    }

    public List<Entry<TestStepFinished, TestStep>> findTestStepFinishedAndTestStepBy(TestCaseStarted testCaseStarted) {
        return findTestStepsFinishedBy(testCaseStarted).stream()
                .map(testStepFinished -> findTestStepBy(testStepFinished).map(testStep -> new SimpleEntry<>(testStepFinished, testStep)))
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(toList());
    }

    public void update(Envelope envelope) {
        envelope.getTestRunStarted().ifPresent(this::updateTestRunStarted);
        envelope.getTestRunFinished().ifPresent(this::updateTestRunFinished);
        envelope.getTestCaseStarted().ifPresent(this::updateTestCaseStarted);
        envelope.getTestCaseFinished().ifPresent(this::updateTestCaseFinished);
        envelope.getTestStepFinished().ifPresent(this::updateTestStepFinished);
        envelope.getGherkinDocument().ifPresent(this::updateGherkinDocument);
        envelope.getPickle().ifPresent(this::updatePickle);
        envelope.getTestCase().ifPresent(this::updateTestCase);
    }

    private Optional<GherkinDocumentElements> findGherkinAstNodesBy(Pickle pickle) {
        requireNonNull(pickle);
        List<String> astNodeIds = pickle.getAstNodeIds();
        String pickleAstNodeId = astNodeIds.get(astNodeIds.size() - 1);
        return Optional.ofNullable(gherkinAstNodesById.get(pickleAstNodeId));
    }

    private Optional<GherkinDocumentElements> findGherkinAstNodesBy(TestCaseStarted testCaseStarted) {
        return findPickleBy(testCaseStarted)
                .flatMap(this::findGherkinAstNodesBy);
    }

    private void updateTestCaseStarted(TestCaseStarted testCaseStarted) {
        this.testCaseStarted.add(testCaseStarted);
    }

    private void updateTestCase(TestCase event) {
        this.testCaseById.put(event.getId(), event);
        event.getTestSteps().forEach(testStep -> testStepById.put(testStep.getId(), testStep));
    }

    private void updatePickle(Pickle event) {
        this.pickleById.put(event.getId(), event);
        event.getSteps().forEach(pickleStep -> pickleStepById.put(pickleStep.getId(), pickleStep));
    }

    private void updateGherkinDocument(GherkinDocument document) {
        document.getFeature().ifPresent(feature -> updateFeature(document, feature));
    }

    private void updateFeature(GherkinDocument document, Feature feature) {
        feature.getChildren()
                .forEach(featureChild -> {
                    featureChild.getBackground().ifPresent(background -> updateSteps(background.getSteps()));
                    featureChild.getScenario().ifPresent(scenario -> updateScenario(document, feature, null, scenario));
                    featureChild.getRule().ifPresent(rule -> rule.getChildren().forEach(ruleChild -> {
                        ruleChild.getBackground().ifPresent(background -> updateSteps(background.getSteps()));
                        ruleChild.getScenario().ifPresent(scenario -> updateScenario(document, feature, rule, scenario));
                    }));
                });
    }

    private void updateSteps(List<Step> steps) {
        steps.forEach(step -> stepById.put(step.getId(), step));
    }

    private void updateTestStepFinished(TestStepFinished event) {
        this.testStepsFinishedByTestCaseStartedId.compute(event.getTestCaseStartedId(), updateList(event));
    }

    private void updateTestCaseFinished(TestCaseFinished event) {
        this.testCaseFinishedByTestCaseStartedId.put(event.getTestCaseStartedId(), event);
    }

    private void updateTestRunFinished(TestRunFinished event) {
        this.testRunFinished = event;
    }

    private void updateTestRunStarted(TestRunStarted event) {
        this.testRunStarted = event;
    }

    private void updateScenario(GherkinDocument document, Feature feature, Rule rule, Scenario scenario) {
        this.gherkinAstNodesById.put(scenario.getId(), new GherkinDocumentElements(document, feature, rule, scenario));
        updateSteps(scenario.getSteps());

        List<Examples> examples = scenario.getExamples();
        for (int examplesIndex = 0; examplesIndex < examples.size(); examplesIndex++) {
            Examples currentExamples = examples.get(examplesIndex);
            List<TableRow> tableRows = currentExamples.getTableBody();
            for (int exampleIndex = 0; exampleIndex < tableRows.size(); exampleIndex++) {
                TableRow currentExample = tableRows.get(exampleIndex);
                gherkinAstNodesById.put(currentExample.getId(), new GherkinDocumentElements(document, feature, rule, scenario, examplesIndex, currentExamples, exampleIndex, currentExample));
            }
        }
    }

    private <K, E> BiFunction<K, List<E>, List<E>> updateList(E element) {
        return (key, existing) -> {
            if (existing != null) {
                existing.add(element);
                return existing;
            }
            List<E> list = new ArrayList<>();
            list.add(element);
            return list;
        };
    }
}