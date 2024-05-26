package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;

import java.util.Objects;
import java.util.Optional;
import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * A structure containing all ancestors of a given
 * {@linkplain GherkinDocument GherkinDocument  element} or
 * {@link  io.cucumber.messages.types.Pickle}.
 * <p>
 *
 * @see LineageReducer
 */
class Lineage {

    private final GherkinDocument document;
    private final Feature feature;
    private final Rule rule;
    private final Scenario scenario;
    private final Examples examples;
    private final Integer examplesIndex;
    private final TableRow example;
    private final Integer exampleIndex;

    Lineage(GherkinDocument document) {
        this(document, null, null, null, null, null, null, null);
    }

    Lineage(Lineage parent, Feature feature) {
        this(parent.document, feature, null, null, null, null, null, null);
    }

    Lineage(Lineage parent, Rule rule) {
        this(parent.document, parent.feature, rule, null, null, null, null, null);
    }

    Lineage(Lineage parent, Scenario scenario) {
        this(parent.document, parent.feature, parent.rule, scenario, null, null, null, null);
    }

    Lineage(Lineage parent, Examples examples, int examplesIndex) {
        this(parent.document, parent.feature, parent.rule, parent.scenario, examples, examplesIndex, null, null);
    }

    Lineage(Lineage parent, TableRow example, int exampleIndex) {
        this(parent.document, parent.feature, parent.rule, parent.scenario, parent.examples, parent.examplesIndex, example, exampleIndex);
    }

    private Lineage(GherkinDocument document, Feature feature, Rule rule, Scenario scenario, Examples examples, Integer examplesIndex, TableRow example, Integer exampleIndex) {
        this.document = requireNonNull(document);
        this.feature = feature;
        this.rule = rule;
        this.scenario = scenario;
        this.examples = examples;
        this.examplesIndex = examplesIndex;
        this.example = example;
        this.exampleIndex = exampleIndex;
    }

    GherkinDocument document() {
        return document;
    }

    Optional<Feature> feature() {
        return Optional.ofNullable(feature);
    }

    Optional<Rule> rule() {
        return Optional.ofNullable(rule);
    }

    Optional<Scenario> scenario() {
        return Optional.ofNullable(scenario);
    }

    Optional<Examples> examples() {
        return Optional.ofNullable(examples);
    }

    Optional<TableRow> example() {
        return Optional.ofNullable(example);
    }

    Optional<Integer> examplesIndex() {
        return Optional.ofNullable(examplesIndex);
    }

    Optional<Integer> exampleIndex() {
        return Optional.ofNullable(exampleIndex);
    }

    <T> LineageReducer reduce(Supplier<LineageCollector<T>> collector) {
        return new LineageReducerDescending(collector);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Lineage that = (Lineage) o;
        return document.equals(that.document) && feature.equals(that.feature) && Objects.equals(rule, that.rule) && scenario.equals(that.scenario) && Objects.equals(examples, that.examples) && Objects.equals(example, that.example) && Objects.equals(examplesIndex, that.examplesIndex) && Objects.equals(exampleIndex, that.exampleIndex);
    }

    @Override
    public int hashCode() {
        return Objects.hash(document, feature, rule, scenario, examples, example, examplesIndex, exampleIndex);
    }
}
