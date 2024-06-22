package io.cucumber.query;

import io.cucumber.messages.types.Pickle;

import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * Visit the {@link Lineage} of a {@linkplain io.cucumber.messages.types.GherkinDocument GherkinDocument element}
 * or {@link Pickle} and reduce it.
 * <p>
 * Because we are using messages we can not express the hierarchy of elements in
 * a {@link io.cucumber.messages.types.GherkinDocument} programmatically as a
 * tree of nodes. But we can still express the operations that would be typically
 * done this way as an operation on the lineage of those messages.
 *
 * @param <T> the type reduced to.
 */
interface LineageReducer<T> {

    static <T> LineageReducer<T> descending(Supplier<? extends LineageCollector<T>> collector) {
        return new LineageReducerDescending<>(collector);
    }

    T reduce(Lineage lineage);

    T reduce(Lineage lineage, Pickle pickle);

}
