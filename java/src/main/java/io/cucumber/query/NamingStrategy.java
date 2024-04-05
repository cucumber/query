package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import static io.cucumber.query.NamingStrategy.ExampleName.NUMBER;
import static io.cucumber.query.NamingStrategy.FeatureName.INCLUDE;

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
         * Use the name of the pickle associated with the example.
         */
        PICKLE
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

    abstract String name(GherkinDocumentElements elements, Pickle pickle);


    private static String exampleNumber(GherkinDocumentElements elements, Integer index) {
        String examplesPrefix = elements.examplesIndex()
                .map(examplesIndex -> examplesIndex + 1)
                .map(examplesIndex -> examplesIndex + ".")
                .orElse("");
        return "Example #" + examplesPrefix + (index + 1);
    }

    private static String join(List<String> pieces) {
        return pieces.stream()
                .filter(s -> !s.isEmpty())
                .collect(Collectors.joining(" - "));
    }

    private static class ShortNamingStrategy extends NamingStrategy {
        private final ExampleName exampleName;

        private ShortNamingStrategy(ExampleName exampleName) {
            this.exampleName = exampleName;
        }

        String name(GherkinDocumentElements elements, Pickle pickle) {
            return elements.exampleIndex()
                    .filter(index -> exampleName == NUMBER)
                    .map(index -> exampleNumber(elements, index))
                    .orElseGet(pickle::getName);
        }

    }

    private static class LongNamingStrategy extends NamingStrategy {
        private final FeatureName featureName;
        private final ExampleName exampleName;

        private LongNamingStrategy(FeatureName featureName, ExampleName exampleName) {
            this.featureName = featureName;
            this.exampleName = exampleName;
        }

        String name(GherkinDocumentElements elements, Pickle pickle) {
            List<String> pieces = new ArrayList<>();
            elements.feature().map(Feature::getName)
                    .filter(feature -> featureName == INCLUDE)
                    .ifPresent(pieces::add);
            elements.rule().map(Rule::getName)
                    .ifPresent(pieces::add);
            elements.scenario().map(Scenario::getName)
                    .ifPresent(pieces::add);
            elements.examples().map(Examples::getName)
                    .ifPresent(pieces::add);
            elements.exampleIndex()
                    .map(index -> exampleName == NUMBER ? exampleNumber(elements, index) : pickle.getName())
                    .ifPresent(pieces::add);
            return join(pieces);
        }
    }

    public static class Builder {
        private final Strategy strategy;
        private FeatureName featureName = INCLUDE;
        private ExampleName exampleName = NUMBER;

        public Builder(Strategy strategy) {
            this.strategy = strategy;
        }

        public Builder exampleName(ExampleName exampleName) {
            this.exampleName = exampleName;
            return this;
        }

        public Builder featureName(FeatureName featureName) {
            this.featureName = featureName;
            return this;
        }

        public NamingStrategy build() {
            if (strategy == Strategy.SHORT) {
                return new ShortNamingStrategy(exampleName);
            }
            return new LongNamingStrategy(featureName, exampleName);

        }
    }
}
