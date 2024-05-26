package io.cucumber.query;

import io.cucumber.messages.types.Pickle;

import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * TODO: Explain how this solves the lack of an AST that has nodes.
 *
 * @param <T>
 */
interface LineageReducerStrategy<T> {

    T reduce(Lineage lineage);

    T reduce(Lineage lineage, Pickle pickle);

    class Descending<T> implements LineageReducerStrategy<T> {

        private final Supplier<LineageReducer<T>> reducerSupplier;

        Descending(Supplier<LineageReducer<T>> reducerSupplier) {
            this.reducerSupplier = requireNonNull(reducerSupplier);
        }

        @Override
        public T reduce(Lineage lineage) {
            LineageReducer<T> reducer = reducerSupplier.get();
            reduceAddLineage(reducer, lineage);
            return reducer.finish();
        }

        @Override
        public T reduce(Lineage lineage, Pickle pickle) {
            LineageReducer<T> reducer = reducerSupplier.get();
            reduceAddLineage(reducer, lineage);
            reducer.add(pickle);
            return reducer.finish();
        }

        private static <T> void reduceAddLineage(LineageReducer<T> reducer, Lineage lineage) {
            lineage.feature().ifPresent(reducer::add);
            lineage.rule().ifPresent(reducer::add);
            lineage.scenario().ifPresent(reducer::add);
            lineage.examples().ifPresent(examples -> reducer.add(examples, lineage.examplesIndex().orElse(0)));
            lineage.example().ifPresent(example -> reducer.add(example, lineage.exampleIndex().orElse(0)));
        }
    }
}
