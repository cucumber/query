package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;
import io.cucumber.query.NamingStrategy.ExampleName;
import io.cucumber.query.NamingStrategy.FeatureName;
import io.cucumber.query.NamingStrategy.Strategy;

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import static io.cucumber.query.NamingStrategy.FeatureName.INCLUDE;
import static io.cucumber.query.NamingStrategy.Strategy.SHORT;

/**
 * Names {@linkplain io.cucumber.messages.types.GherkinDocument GherkinDocument element}
 * or {@link Pickle}.
 *
 * @see NamingStrategy
 */
class NamingCollector implements LineageCollector<String> {

    private final Deque<String> parts = new ArrayDeque<>();
    private final CharSequence delimiter = " - ";
    private final Strategy strategy;
    private final FeatureName featureName;
    private final ExampleName exampleName;

    private String scenarioName;
    private boolean isExample;
    private int examplesIndex;

    static Supplier<NamingCollector> of(Strategy strategy, FeatureName featureName, ExampleName exampleName) {
        return () -> new NamingCollector(strategy, featureName, exampleName);
    }

    private NamingCollector(Strategy strategy, FeatureName featureName, ExampleName exampleName) {
        this.strategy = strategy;
        this.featureName = featureName;
        this.exampleName = exampleName;
    }

    public void add(Feature feature) {
        if (featureName == INCLUDE || strategy == SHORT) {
            parts.add(feature.getName());
        }
    }

    public void add(Rule rule) {
        parts.add(rule.getName());
    }

    public void add(Scenario scenario) {
        scenarioName = scenario.getName();
        parts.add(scenarioName);
    }

    public void add(Examples examples, int index) {
        parts.add(examples.getName());
        this.examplesIndex = index;
    }

    public void add(TableRow example, int index) {
        isExample = true;
        parts.add("#" + (examplesIndex + 1) + "." + (index + 1));
    }

    public void add(Pickle pickle) {
        String pickleName = pickle.getName();

        // Case 0: Pickles with an empty a lineage
        if (scenarioName == null){
            parts.add(pickleName);
            return;
        }

        // Case 1: Pickles from a scenario outline
        if (isExample) {
            switch (exampleName) {
                case NUMBER:
                    break;
                case NUMBER_AND_PICKLE_IF_PARAMETERIZED:
                    boolean parameterized = !scenarioName.equals(pickleName);
                    if (parameterized) {
                        String exampleNumber = parts.removeLast();
                        parts.add(exampleNumber + ": " + pickleName);
                    }
                    break;
                case PICKLE:
                    parts.removeLast(); // Remove example number
                    parts.add(pickleName);
                    break;
            }
        }
        // Case 2: Pickles from a scenario
        // Nothing to do, scenario name and pickle name are the same.
    }

    @Override
    public String finish() {
        if (strategy == SHORT) {
            return parts.getLast();
        }
        return parts.stream()
                .filter(s -> !s.isEmpty())
                .collect(Collectors.joining(delimiter));
    }
}
