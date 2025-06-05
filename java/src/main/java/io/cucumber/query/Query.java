package io.cucumber.query;

import io.cucumber.messages.Convertor;
import io.cucumber.messages.types.Attachment;
import io.cucumber.messages.types.Envelope;
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
import java.util.*;
import java.util.AbstractMap.SimpleEntry;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiFunction;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import static io.cucumber.query.LineageReducer.ascending;
import static java.util.Collections.emptyList;
import static java.util.Comparator.comparing;
import static java.util.Comparator.naturalOrder;
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
 */
public final class Query {
    private static final Map<TestStepResultStatus, Long> ZEROES_BY_TEST_STEP_RESULT_STATUSES = Arrays.stream(TestStepResultStatus.values())
            .collect(Collectors.toMap(identity(), (s) -> 0L));
    private final Comparator<TestStepResult> testStepResultComparator = nullsFirst(comparing(o -> o.getStatus().ordinal()));
    private final Map<String, TestCaseStarted> testCaseStartedById = new ConcurrentHashMap<>();
    private final Map<String, TestCaseFinished> testCaseFinishedByTestCaseStartedId = new ConcurrentHashMap<>();
    private final Map<String, List<TestStepFinished>> testStepsFinishedByTestCaseStartedId = new ConcurrentHashMap<>();
    private final Map<String, Pickle> pickleById = new ConcurrentHashMap<>();
    private final Map<String, TestCase> testCaseById = new ConcurrentHashMap<>();
    private final Map<String, Step> stepById = new ConcurrentHashMap<>();
    private final Map<String, TestStep> testStepById = new ConcurrentHashMap<>();
    private final Map<String, PickleStep> pickleStepById = new ConcurrentHashMap<>();
    private final Map<String, Hook> hookById = new ConcurrentHashMap<>();
    private final Map<String, List<Attachment>> attachmentsByTestCaseStartedId = new ConcurrentHashMap<>();
    private final Map<Object, Lineage> lineageById = new ConcurrentHashMap<>();
    private Meta meta;
    private TestRunStarted testRunStarted;
    private TestRunFinished testRunFinished;

    public Map<TestStepResultStatus, Long> countMostSevereTestStepResultStatus() {
        EnumMap<TestStepResultStatus, Long> results = new EnumMap<>(ZEROES_BY_TEST_STEP_RESULT_STATUSES);
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
        return this.testCaseStartedById.values().stream()
                .sorted(comparing(TestCaseStarted::getTimestamp, timestampComparator))
                .filter(element -> !findTestCaseFinishedBy(element)
                        .filter(TestCaseFinished::getWillBeRetried)
                        .isPresent())
                .collect(toList());
    }
    
    // TODO: Move to Messages, make comparable?
    private final Comparator<Timestamp> timestampComparator = (a, b) -> {
        long x = a.getSeconds();
        long y = b.getSeconds();
        int cmp;
        if (x < y) 
            return -1;
        if(y > x) 
            return 1;

        long x1 = a.getNanos();
        long y1 = b.getNanos();
        if (x1 < y1)
            return -1;
        if(y1 > x1)
            return 1;
        return 0;
    };

    public Map<Optional<Feature>, List<TestCaseStarted>> findAllTestCaseStartedGroupedByFeature() {
        return findAllTestCaseStarted()
                .stream()
                .map(testCaseStarted -> {
                    Optional<Lineage> astNodes = findLineageBy(testCaseStarted);
                    return new SimpleEntry<>(astNodes, testCaseStarted);
                })
                // Sort entries by gherkin document URI for consistent ordering
                .sorted(comparing(
                        entry -> entry.getKey()
                                .flatMap(nodes -> nodes.document().getUri())
                                .orElse(null),
                        nullsFirst(naturalOrder())
                ))
                .map(entry -> {
                    // Unpack the now sorted entries
                    Optional<Feature> feature = entry.getKey().flatMap(Lineage::feature);
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

    public List<Attachment> findAttachmentsBy(TestStepFinished testStepFinished) {
        requireNonNull(testStepFinished);
        return attachmentsByTestCaseStartedId.getOrDefault(testStepFinished.getTestCaseStartedId(), emptyList()).stream()
                .filter(attachment -> attachment.getTestStepId()
                        .map(testStepId -> testStepFinished.getTestStepId().equals(testStepId))
                        .orElse(false))
                .collect(toList());
    }

    public Optional<Feature> findFeatureBy(TestCaseStarted testCaseStarted) {
        return findLineageBy(testCaseStarted).flatMap(Lineage::feature);
    }

    public Optional<Hook> findHookBy(TestStep testStep) {
        requireNonNull(testStep);
        return testStep.getHookId()
                .map(hookById::get);
    }

    public Optional<Meta> findMeta() {
        return ofNullable(meta);
    }

    public Optional<TestStepResult> findMostSevereTestStepResultBy(TestCaseStarted testCaseStarted) {
        requireNonNull(testCaseStarted);
        return findTestStepsFinishedBy(testCaseStarted)
                .stream()
                .map(TestStepFinished::getTestStepResult)
                .max(testStepResultComparator);
    }

    public String findNameOf(GherkinDocument element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(Feature element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(Rule element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(Scenario element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(Examples element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(TableRow element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseThrow(createElementWasNotPartOfThisQueryObject());
    }

    public String findNameOf(Pickle element, NamingStrategy namingStrategy) {
        requireNonNull(element);
        requireNonNull(namingStrategy);
        return reduceLinageOf(element, namingStrategy)
                .orElseGet(element::getName);
    }

    private static Supplier<IllegalArgumentException> createElementWasNotPartOfThisQueryObject() {
        return () -> new IllegalArgumentException("Element was not part of this query object");
    }

    public <T> Optional<T> reduceLinageOf(GherkinDocument element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(Feature element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(Rule element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(Scenario element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(Examples element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(TableRow element, LineageReducer<T> reducer) {
        requireNonNull(element);
        requireNonNull(reducer);
        return findLineageBy(element)
                .map(reducer::reduce);
    }

    public <T> Optional<T> reduceLinageOf(Pickle pickle, LineageReducer<T> reducer) {
        requireNonNull(pickle);
        requireNonNull(reducer);
        return findLineageBy(pickle)
                .map(lineage -> reducer.reduce(lineage, pickle));
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
                .map(TestCase::getPickleId)
                .map(pickleById::get);
    }

    public Optional<Pickle> findPickleBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return findTestCaseBy(testStepStarted)
                .map(TestCase::getPickleId)
                .map(pickleById::get);
    }

    public Optional<PickleStep> findPickleStepBy(TestStep testStep) {
        requireNonNull(testStep);
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

    public Optional<TestCase> findTestCaseBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return findTestCaseStartedBy(testStepStarted)
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

    public Optional<TestCaseStarted> findTestCaseStartedBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        String testCaseStartedId = testStepStarted.getTestCaseStartedId();
        return ofNullable(testCaseStartedById.get(testCaseStartedId));
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

    public Optional<TestStep> findTestStepBy(TestStepStarted testStepStarted) {
        requireNonNull(testStepStarted);
        return ofNullable(testStepById.get(testStepStarted.getTestStepId()));
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
        envelope.getMeta().ifPresent(this::updateMeta);
        envelope.getTestRunStarted().ifPresent(this::updateTestRunStarted);
        envelope.getTestRunFinished().ifPresent(this::updateTestRunFinished);
        envelope.getTestCaseStarted().ifPresent(this::updateTestCaseStarted);
        envelope.getTestCaseFinished().ifPresent(this::updateTestCaseFinished);
        envelope.getTestStepFinished().ifPresent(this::updateTestStepFinished);
        envelope.getGherkinDocument().ifPresent(this::updateGherkinDocument);
        envelope.getPickle().ifPresent(this::updatePickle);
        envelope.getTestCase().ifPresent(this::updateTestCase);
        envelope.getHook().ifPresent(this::updateHook);
        envelope.getAttachment().ifPresent(this::updateAttachment);
    }

    public Optional<Lineage> findLineageBy(GherkinDocument element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element.getUri()));
    }

    public Optional<Lineage> findLineageBy(Feature element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element));
    }

    public Optional<Lineage> findLineageBy(Rule element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Scenario element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Examples element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(TableRow element) {
        requireNonNull(element);
        return Optional.ofNullable(lineageById.get(element.getId()));
    }

    public Optional<Lineage> findLineageBy(Pickle pickle) {
        requireNonNull(pickle);
        List<String> astNodeIds = pickle.getAstNodeIds();
        String pickleAstNodeId = astNodeIds.get(astNodeIds.size() - 1);
        return Optional.ofNullable(lineageById.get(pickleAstNodeId));
    }

    public Optional<Lineage> findLineageBy(TestCaseStarted testCaseStarted) {
        return findPickleBy(testCaseStarted)
                .flatMap(this::findLineageBy);
    }

    private void updateAttachment(Attachment attachment) {
        attachment.getTestCaseStartedId()
                .ifPresent(testCaseStartedId -> this.attachmentsByTestCaseStartedId.compute(testCaseStartedId, updateList(attachment)));
    }

    private void updateHook(Hook hook) {
        this.hookById.put(hook.getId(), hook);
    }

    private void updateTestCaseStarted(TestCaseStarted testCaseStarted) {
        this.testCaseStartedById.put(testCaseStarted.getId(), testCaseStarted);
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
        lineageById.putAll(Lineages.of(document));
        document.getFeature().ifPresent(this::updateFeature);
    }

    private void updateFeature(Feature feature) {
        feature.getChildren()
                .forEach(featureChild -> {
                    featureChild.getBackground().ifPresent(background -> updateSteps(background.getSteps()));
                    featureChild.getScenario().ifPresent(this::updateScenario);
                    featureChild.getRule().ifPresent(rule -> rule.getChildren().forEach(ruleChild -> {
                        ruleChild.getBackground().ifPresent(background -> updateSteps(background.getSteps()));
                        ruleChild.getScenario().ifPresent(this::updateScenario);
                    }));
                });
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

    private void updateScenario(Scenario scenario) {
        updateSteps(scenario.getSteps());
    }

    private void updateSteps(List<Step> steps) {
        steps.forEach(step -> stepById.put(step.getId(), step));
    }

    private void updateMeta(Meta event) {
        this.meta = event;
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
