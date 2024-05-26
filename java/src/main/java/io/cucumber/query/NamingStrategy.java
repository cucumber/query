package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.query.LineageVisitingStrategy.CollectingVisitor;
import io.cucumber.query.LineageVisitingStrategy.All;
import io.cucumber.query.LineageVisitingStrategy.Parent;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import static io.cucumber.query.NamingStrategy.ExampleName.NUMBER;
import static io.cucumber.query.NamingStrategy.FeatureName.INCLUDE;
import static java.util.Objects.requireNonNull;

/**
 * Names {@link Pickle Pickles} in a {@link GherkinDocument}.
 * <p>
 * Pickles in a Gherkin document have a name. But represented
 * without the structure of a Gherkin document (e.g. in a flat xml report),
 * these names can lose their meaning. The long naming strategy solves this
 * problem by prefixing an elements name with the names of all its ancestors,
 * optionally including the feature name.
 * <p>
 * Furthermore, Pickles derived from an example can be named in two ways.
 * Either by their example number (e.g. {@code Example #3.14}) or by their
 * pickle name. If a parameterized pickle name is used, the latter may be
 * preferable.
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
 * With the long strategy, using example numbers the pickles in this example would be named:
 * <ul>
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are passing - Example #1.1
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are passing - Example #1.2
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are failing - Example #2.1
 *     <li>Examples Tables - Eating &lt;eat&gt; cucumbers - These are failing - Example #2.2
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
public abstract class NamingStrategy {

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
         * Number examples, for example {@code Example #3.14}
         */
        NUMBER,

        /**
         * Use the name of the pickle associated with the example. For example
         * {@coe Eating 6 cucumbers}.
         */
        PICKLE,

        /**
         * Number examples, and if the pickle name is parameterized include it
         * too. For example {@code Example #3.14 - Eating 6 cucumbers}.
         */
        // TODO: Test this, needs https://github.com/cucumber/compatibility-kit/issues/96
        NUMBER_AND_PICKLE_NAME_IF_PARAMETERIZED
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

    abstract String name(Lineage elements, Pickle pickle);

    static class NamingVisitor implements CollectingVisitor<String> {

        private final List<String> pieces = new ArrayList<>();
        private final CharSequence delimiter = " - ";
        private final FeatureName featureName;
        private final ExampleName exampleName;

        private String scenarioName;
        private String exampleNumber;

        NamingVisitor(FeatureName featureName, ExampleName exampleName) {
            this.featureName = featureName;
            this.exampleName = exampleName;
        }

        public void accept(Feature feature) {
            if (featureName == INCLUDE) {
                pieces.add(feature.getName());
            }
        }

        public void accept(Rule rule) {
            pieces.add(rule.getName());
        }

        public void accept(Scenario scenario) {
            scenarioName = scenario.getName();
            pieces.add(scenarioName);
        }

        public void accept(Examples examples) {
            pieces.add(examples.getName());
        }

        public void accept(int examplesIndex, int exampleIndex) {
            switch (exampleName) {
                case NUMBER:
                case NUMBER_AND_PICKLE_NAME_IF_PARAMETERIZED:
                    exampleNumber = "Example #" + (examplesIndex + 1) + "." + (exampleIndex + 1);
                    pieces.add(exampleNumber);
            }
        }

        public void accept(Pickle pickle) {
            String pickleName = pickle.getName();
            if (scenarioName != null && scenarioName.equals(pickleName)) {
                return;
            }
            switch (exampleName) {
                case NUMBER:
                    if (exampleNumber != null) {
                        return;
                    }
                case NUMBER_AND_PICKLE_NAME_IF_PARAMETERIZED:
                case PICKLE:
                    pieces.add(pickleName);
            }
        }

        @Override
        public String collect() {
            return pieces.stream()
                    .filter(s -> !s.isEmpty())
                    .collect(Collectors.joining(delimiter));
        }
    }

    public static class Builder {

        private final Strategy strategy;
        private FeatureName featureName = INCLUDE;
        private ExampleName exampleName = NUMBER;

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
            Supplier<CollectingVisitor<String>> visitor = () -> new NamingVisitor(featureName, exampleName);
            LineageVisitingStrategy<String> strategy = createStrategy(visitor);
            return new ElementLineageVisitingStrategyAdaptor(strategy);

        }

        private LineageVisitingStrategy<String> createStrategy(Supplier<CollectingVisitor<String>> visitor) {
            if (strategy == Strategy.SHORT) {
                return new Parent<>(visitor);
            }
            return new All<>(visitor);
        }

        private static class ElementLineageVisitingStrategyAdaptor extends NamingStrategy {
            private final LineageVisitingStrategy<String> delegate;

            ElementLineageVisitingStrategyAdaptor(LineageVisitingStrategy<String> delegate) {
                this.delegate = delegate;
            }

            @Override
            String name(Lineage elements, Pickle pickle) {
                return delegate.visit(elements, pickle);
            }
        }
    }


}
