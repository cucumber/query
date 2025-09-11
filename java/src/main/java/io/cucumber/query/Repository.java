package io.cucumber.query;

import io.cucumber.messages.types.Attachment;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Hook;
import io.cucumber.messages.types.Meta;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.PickleStep;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.Step;
import io.cucumber.messages.types.StepDefinition;
import io.cucumber.messages.types.Suggestion;
import io.cucumber.messages.types.TestCase;
import io.cucumber.messages.types.TestCaseFinished;
import io.cucumber.messages.types.TestCaseStarted;
import io.cucumber.messages.types.TestRunFinished;
import io.cucumber.messages.types.TestRunStarted;
import io.cucumber.messages.types.TestStep;
import io.cucumber.messages.types.TestStepFinished;
import io.cucumber.messages.types.TestStepStarted;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.BiFunction;

import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_ATTACHMENTS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_GHERKIN_DOCUMENTS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_HOOKS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_STEP_DEFINITIONS;
import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_SUGGESTIONS;

/**
 * A write only repository of Cucumber Messages.
 * <p>
 * This class is effectively a simple in memory database. It can be updated in
 * through the {@link #update(Envelope)} method, and be queried by {@link Query}.
 */
public final class Repository {
    private final Set<RepositoryFeature> features;

    final Map<String, TestCaseStarted> testCaseStartedById = new LinkedHashMap<>();
    final Map<String, TestCaseFinished> testCaseFinishedByTestCaseStartedId = new LinkedHashMap<>();
    final Map<String, List<TestStepFinished>> testStepsFinishedByTestCaseStartedId = new LinkedHashMap<>();
    final Map<String, List<TestStepStarted>> testStepsStartedByTestCaseStartedId = new LinkedHashMap<>();
    final Map<String, Pickle> pickleById = new LinkedHashMap<>();
    final Map<String, TestCase> testCaseById = new LinkedHashMap<>();
    final Map<String, Step> stepById = new LinkedHashMap<>();
    final Map<String, TestStep> testStepById = new LinkedHashMap<>();
    final Map<String, PickleStep> pickleStepById = new LinkedHashMap<>();
    final Map<String, Hook> hookById = new LinkedHashMap<>();
    final Map<String, List<Attachment>> attachmentsByTestCaseStartedId = new LinkedHashMap<>();
    final Map<Object, Lineage> lineageById = new HashMap<>();
    final Map<String, StepDefinition> stepDefinitionById = new LinkedHashMap<>();
    final Map<String, List<Suggestion>> suggestionsByPickleStepId = new LinkedHashMap<>();

    Meta meta;
    TestRunStarted testRunStarted;
    TestRunFinished testRunFinished;

    private Repository(Set<RepositoryFeature> features) {
        this.features = features;
    }

    public static Builder builder() {
        return new Builder();
    }

    public enum RepositoryFeature {

        /**
         * Include {@link Attachment} messages.
         * <p>
         * Disable to reduce memory usage.
         */
        INCLUDE_ATTACHMENTS,

        /**
         * Include {@link io.cucumber.messages.types.GherkinDocument} messages.
         * <p>
         * Disable to reduce memory usage.
         */
        INCLUDE_GHERKIN_DOCUMENTS,

        /**
         * Include {@link Hook} messages.
         * <p>
         * Disable to reduce memory usage.
         */
        INCLUDE_HOOKS,

        /**
         * Include {@link StepDefinition} messages.
         * <p>
         * Disable to reduce memory usage.
         */
        INCLUDE_STEP_DEFINITIONS,

        /**
         * Include {@link Suggestion} messages.
         * <p>
         * Disable to reduce memory usage.
         */
        INCLUDE_SUGGESTIONS,
    }

    public static class Builder {
        private final EnumSet<RepositoryFeature> features = EnumSet.noneOf(RepositoryFeature.class);
        
        private Builder(){
            
        }
        /**
         * Toggles a given feature.
         */
        public Builder feature(RepositoryFeature feature, boolean enabled) {
            if (enabled) {
                features.add(feature);
            } else {
                features.remove(feature);
            }
            return this;
        }

        public Repository build() {
            return new Repository(features);
        }
    }

    public void update(Envelope envelope) {
        envelope.getMeta().ifPresent(this::updateMeta);
        envelope.getTestRunStarted().ifPresent(this::updateTestRunStarted);
        envelope.getTestRunFinished().ifPresent(this::updateTestRunFinished);
        envelope.getTestCaseStarted().ifPresent(this::updateTestCaseStarted);
        envelope.getTestCaseFinished().ifPresent(this::updateTestCaseFinished);
        envelope.getTestStepStarted().ifPresent(this::updateTestStepStarted);
        envelope.getTestStepFinished().ifPresent(this::updateTestStepFinished);
        envelope.getPickle().ifPresent(this::updatePickle);
        envelope.getTestCase().ifPresent(this::updateTestCase);

        if (features.contains(INCLUDE_GHERKIN_DOCUMENTS)) {
            envelope.getGherkinDocument().ifPresent(this::updateGherkinDocument);
        }
        if (features.contains(INCLUDE_STEP_DEFINITIONS)) {
            envelope.getStepDefinition().ifPresent(this::updateStepDefinition);
        }
        if (features.contains(INCLUDE_HOOKS)) {
            envelope.getHook().ifPresent(this::updateHook);
        }
        if (features.contains(INCLUDE_ATTACHMENTS)) {
            envelope.getAttachment().ifPresent(this::updateAttachment);
        }
        if (features.contains(INCLUDE_SUGGESTIONS)) {
            envelope.getSuggestion().ifPresent(this::updateSuggestions);
        }
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

    private void updateFeature(io.cucumber.messages.types.Feature feature) {
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

    private void updateTestStepStarted(TestStepStarted event) {
        this.testStepsStartedByTestCaseStartedId.compute(event.getTestCaseStartedId(), updateList(event));
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

    private void updateStepDefinition(StepDefinition event) {
        this.stepDefinitionById.put(event.getId(), event);
    }

    private void updateSteps(List<Step> steps) {
        steps.forEach(step -> stepById.put(step.getId(), step));
    }

    private void updateSuggestions(Suggestion event) {
        this.suggestionsByPickleStepId.compute(event.getPickleStepId(), updateList(event));
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
