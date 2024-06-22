package io.cucumber.query;

import io.cucumber.messages.types.Pickle;

import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * Reduces the lineage of a Gherkin document element in descending order.
 *
 * @param <T> type to which the lineage is reduced.
 */
class LineageReducerDescending<T> implements LineageReducer<T> {

    private final Supplier<? extends LineageCollector<T>> reducerSupplier;

    LineageReducerDescending(Supplier<? extends LineageCollector<T>> reducerSupplier) {
        this.reducerSupplier = requireNonNull(reducerSupplier);
    }

    @Override
    public T reduce(Lineage lineage) {
        LineageCollector<T> reducer = reducerSupplier.get();
        reduceAddLineage(reducer, lineage);
        return reducer.finish();
    }

    @Override
    public T reduce(Lineage lineage, Pickle pickle) {
        LineageCollector<T> reducer = reducerSupplier.get();
        reduceAddLineage(reducer, lineage);
        reducer.add(pickle);
        return reducer.finish();
    }

    private static <T> void reduceAddLineage(LineageCollector<T> reducer, Lineage lineage) {
        reducer.add(lineage.document());
        lineage.feature().ifPresent(reducer::add);
        lineage.rule().ifPresent(reducer::add);
        lineage.scenario().ifPresent(reducer::add);
        lineage.examples().ifPresent(examples -> reducer.add(examples, lineage.examplesIndex().orElse(0)));
        lineage.example().ifPresent(example -> reducer.add(example, lineage.exampleIndex().orElse(0)));
    }
}
