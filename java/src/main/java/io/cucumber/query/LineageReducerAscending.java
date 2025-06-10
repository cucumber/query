package io.cucumber.query;

import io.cucumber.messages.types.Pickle;

import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * Reduces the lineage of a Gherkin document element in ascending order.
 *
 * @param <T> type to which the lineage is reduced.
 */
class LineageReducerAscending<T> implements LineageReducer<T> {

    private final Supplier<? extends Collector<T>> collectorSupplier;

    LineageReducerAscending(Supplier<? extends Collector<T>> collectorSupplier) {
        this.collectorSupplier = requireNonNull(collectorSupplier);
    }

    @Override
    public T reduce(Lineage lineage) {
        Collector<T> collector = collectorSupplier.get();
        reduceAddLineage(collector, lineage);
        return collector.finish();
    }

    @Override
    public T reduce(Lineage lineage, Pickle pickle) {
        Collector<T> collector = collectorSupplier.get();
        collector.add(pickle);
        reduceAddLineage(collector, lineage);
        return collector.finish();
    }

    private static <T> void reduceAddLineage(Collector<T> reducer, Lineage lineage) {
        lineage.example().ifPresent(example -> reducer.add(example, lineage.exampleIndex().orElse(0)));
        lineage.examples().ifPresent(examples -> reducer.add(examples, lineage.examplesIndex().orElse(0)));
        lineage.scenario().ifPresent(reducer::add);
        lineage.rule().ifPresent(reducer::add);
        lineage.feature().ifPresent(reducer::add);
        reducer.add(lineage.document());
    }
}
