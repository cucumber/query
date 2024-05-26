package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.FeatureChild;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.RuleChild;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.Consumer;

class Lineages {

    static Map<Object, Lineage> of(GherkinDocument document) {
        Map<Object, Lineage> elements = new HashMap<>();
        Lineage lineage = new Lineage(document);
        elements.put(document, lineage);
        document.getFeature().ifPresent(ofFeature(lineage, elements));
        return elements;
    }

    private static Consumer<Feature> ofFeature(Lineage parent, Map<Object, Lineage> elements) {
        return feature -> {
            Lineage lineage = new Lineage(parent, feature);
            feature.getChildren().forEach(ofFeatureChild(lineage, elements));
        };
    }

    private static Consumer<FeatureChild> ofFeatureChild(Lineage parent, Map<Object, Lineage> elements) {
        return featureChild -> {
            featureChild.getScenario().ifPresent(ofScenario(parent, elements));
            featureChild.getRule().ifPresent(ofRule(parent, elements));
        };
    }

    private static Consumer<Rule> ofRule(Lineage parent, Map<Object, Lineage> elements) {
        return rule -> {
            Lineage lineage = new Lineage(parent, rule);
            elements.put(rule.getId(), lineage);
            rule.getChildren().forEach(ofRuleChild(lineage, elements));
        };
    }

    private static Consumer<RuleChild> ofRuleChild(Lineage parent, Map<Object, Lineage> elements) {
        return ruleChild -> ruleChild.getScenario().ifPresent(ofScenario(parent, elements));
    }

    private static Consumer<Scenario> ofScenario(Lineage parent, Map<Object, Lineage> elements) {
        return scenario -> {
            Lineage lineage = new Lineage(parent, scenario);
            elements.put(scenario.getId(), lineage);
            forEachIndexed(scenario.getExamples(), ofExamples(lineage, elements));
        };
    }

    private static BiConsumer<Examples, Integer> ofExamples(Lineage parent, Map<Object, Lineage> elements) {
        return (examples, examplesIndex) -> {
            Lineage lineage = new Lineage(parent, examples, examplesIndex);
            elements.put(examples.getId(), lineage);
            forEachIndexed(examples.getTableBody(), ofExample(lineage, elements));
        };
    }

    private static BiConsumer<TableRow, Integer> ofExample(Lineage parent, Map<Object, Lineage> elements) {
        return (example, exampleIndex) -> {
            Lineage lineage = new Lineage(parent, example, exampleIndex);
            elements.put(example.getId(), lineage);
        };
    }

    private static <T> void forEachIndexed(List<T> items, BiConsumer<T, Integer> consumer) {
        for (int i = 0; i < items.size(); i++) {
            consumer.accept(items.get(i), i);
        }
    }
}