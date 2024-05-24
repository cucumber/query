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

    private static class ShortNamingStrategy extends NamingStrategy {
        private final NamingVisitor visitor;
        private final ExampleName exampleName;

        private ShortNamingStrategy(NamingVisitor visitor, ExampleName exampleName) {
            this.visitor = requireNonNull(visitor);
            this.exampleName = requireNonNull(exampleName);
        }

        String name(GherkinDocumentElements elements, Pickle pickle) {
            return elements.exampleIndex()
                    .filter(index -> exampleName == NUMBER)
                    .map(index -> {
                        Integer examplesIndex = elements.examplesIndex().orElse(0);
                        return visitor.accept(examplesIndex, index);
                    })
                    .orElseGet(() -> visitor.accept(pickle));
        }
    }

    private static class LongNamingStrategy extends NamingStrategy {
        private final NamingVisitor visitor;
        private final CharSequence delimiter;
        private final FeatureName featureName;
        private final ExampleName exampleName;

        private LongNamingStrategy(NamingVisitor visitor, CharSequence delimiter, FeatureName featureName, ExampleName exampleName) {
            this.visitor = requireNonNull(visitor);
            this.delimiter = requireNonNull(delimiter);
            this.featureName = requireNonNull(featureName);
            this.exampleName = requireNonNull(exampleName);
        }

        String name(GherkinDocumentElements elements, Pickle pickle) {
            List<String> pieces = new ArrayList<>();
            elements.feature().map(visitor::accept)
                    .filter(feature -> featureName == INCLUDE)
                    .ifPresent(pieces::add);
            elements.rule().map(visitor::accept)
                    .ifPresent(pieces::add);
            elements.scenario().map(visitor::accept)
                    .ifPresent(pieces::add);
            elements.examples().map(visitor::accept)
                    .ifPresent(pieces::add);
            elements.exampleIndex()
                    .map(index -> {
                        Integer examplesIndex = elements.examplesIndex().orElse(0);
                        return exampleName == NUMBER ? visitor.accept(examplesIndex, index) : visitor.accept(pickle);
                    })
                    .ifPresent(pieces::add);

            return pieces.stream()
                    .filter(s -> !s.isEmpty())
                    .collect(Collectors.joining(delimiter));
        }
    }

    public interface NamingVisitor {
        default String accept(Feature feature) {
            return feature.getName();
        }

        default String accept(Rule rule) {
            return rule.getName();
        }

        default String accept(Scenario scenario) {
            return scenario.getName();
        }

        default String accept(Examples examples) {
            return examples.getName();
        }

        default String accept(int examplesIndex, int exampleIndex) {
            return "Example #" + (examplesIndex + 1) + "." + (exampleIndex + 1);
        }

        default String accept(Pickle pickle) {
            return pickle.getName();
        }
    }

    public static class Builder {
        private static class DefaultNamingVisitor implements NamingVisitor {

        }

        private final Strategy strategy;
        private FeatureName featureName = INCLUDE;
        private ExampleName exampleName = NUMBER;
        private NamingVisitor visitor = new DefaultNamingVisitor();
        private CharSequence delimiter = " - ";

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

        public Builder namingVisitor(NamingVisitor visitor) {
            this.visitor = requireNonNull(visitor);
            return this;
        }
        public Builder delimiter(CharSequence delimiter) {
            this.delimiter = requireNonNull(delimiter);
            return this;
        }

        public NamingStrategy build() {
            if (strategy == Strategy.SHORT) {
                return new ShortNamingStrategy(visitor, exampleName);
            }
            return new LongNamingStrategy(visitor, delimiter, featureName, exampleName);

        }
    }


}
