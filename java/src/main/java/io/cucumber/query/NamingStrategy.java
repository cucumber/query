package io.cucumber.query;

import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;

import static io.cucumber.query.LineageReducer.descending;
import static io.cucumber.query.NamingCollector.of;
import static io.cucumber.query.NamingStrategy.ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED;
import static io.cucumber.query.NamingStrategy.FeatureName.INCLUDE;
import static java.util.Objects.requireNonNull;

/**
 * Names {@link Pickle Pickles} and other elements in a
 * {@link GherkinDocument}.
 * <p>
 * Pickles in a Gherkin document have a name. But represented
 * without the structure of a Gherkin document (e.g. in a flat xml report),
 * these names can lose their meaning. The long naming strategy solves this
 * problem by prefixing an elements name with the names of all its ancestors,
 * optionally including the feature name.
 * <p>
 * Furthermore, Pickles derived from an example can be named in three ways.
 * Either by their example number (e.g. {@code Example #3.14}) or by their
 * pickle name. If a parameterized pickle name is used, a combination of both
 * can be used.
 *
 * <pre>{@code
 * Feature: Examples Tables
 *   Scenario Outline: Eating &lt;eat&gt; cucumbers
 *     Given there are &lt;start&gt; cucumbers
 *     When I eat &lt;eat&gt; cucumbers
 *     Then I should have &lt;left&gt; cucumbers
 *
 *     Examples: These are passing
 *       | start | eat | left |
 *       |    12 |   5 |    7 |
 *       |    20 |   6 |   14 |
 *
 *     Examples: These are failing
 *       | start | eat | left |
 *       |    12 |  20 |    0 |
 *       |     0 |   1 |    0 |
 * }</pre>
 *
 * With the long strategy, using example numbers and the pickle if
 * parameterized, the pickles in this example would be named:
 * <ul>
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are passing - #1.1: Eating 5 cucumbers
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are passing - #1.2: Eating 6 cucumbers
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are failing - #2.1: Eating 20 cucumbers
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are failing - #2.2: Eating 1 cucumbers
 * </ul>
 * <p>
 * And with the short strategy, using pickle names:
 * <ul>
 *     <li>Eating 5 cucumbers
 *     <li>Eating 6 cucumbers
 *     <li>Eating 20 cucumbers
 *     <li>Eating 1 cucumbers
 * </ul>
 */
public abstract class NamingStrategy implements LineageReducer<String> {

    public enum Strategy {
        /**
         * Names an element in a Gherkin document by including all ancestors in the name.
         */
        LONG,

        /**
         * Names an element in a Gherkin document by using only its name.
         */
        SHORT
    }

    public enum ExampleName {
        /**
         * Number examples, for example {@code #1.2}
         */
        NUMBER,

        /**
         * Use the name of the pickle associated with the example. For example
         * {@code Eating 6 cucumbers}.
         */
        PICKLE,

        /**
         * Number examples, and if the pickle name is parameterized include it
         * too. For example {@code #1.2: Eating 6 cucumbers}.
         */
        NUMBER_AND_PICKLE_IF_PARAMETERIZED
    }

    public enum FeatureName {
        /**
         * When using the {@link Strategy#LONG} include the feature name.
         */
        INCLUDE,
        /**
         * When using the {@link Strategy#LONG} do not include the feature name.
         */
        EXCLUDE
    }

    public static Builder strategy(Strategy strategy) {
        return new Builder(strategy);
    }

    private NamingStrategy() {
        // Not for public construction.
        // Could be made a public interface if GherkinDocumentElements had a better API
    }

    public static class Builder {

        private final Strategy strategy;
        private FeatureName featureName = INCLUDE;
        private ExampleName exampleName = NUMBER_AND_PICKLE_IF_PARAMETERIZED;

        public Builder(Strategy strategy) {
            this.strategy = requireNonNull(strategy);
        }

        public Builder exampleName(ExampleName exampleName) {
            this.exampleName = requireNonNull(exampleName);
            return this;
        }

        public Builder featureName(FeatureName featureName) {
            this.featureName = requireNonNull(featureName);
            return this;
        }

        public NamingStrategy build() {
            return new Adaptor(descending(of(strategy, featureName, exampleName)));
        }
    }

    private static class Adaptor extends NamingStrategy {
        private final LineageReducer<String> delegate;

        Adaptor(LineageReducer<String> delegate) {
            this.delegate = delegate;
        }

        @Override
        public String reduce(Lineage lineage) {
            return delegate.reduce(lineage);
        }

        @Override
        public String reduce(Lineage lineage, Pickle pickle) {
            return delegate.reduce(lineage, pickle);
        }
    }
}
