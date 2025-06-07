package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;

import java.util.function.Supplier;

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
public interface LineageReducer<T> {

    static <T> LineageReducer<T> descending(Supplier<? extends Collector<T>> collector) {
        return new LineageReducerDescending<>(collector);
    }
    
    static <T> LineageReducer<T> ascending(Supplier<? extends Collector<T>> collector) {
        return new LineageReducerAscending<>(collector);
    }

    T reduce(Lineage lineage);

    T reduce(Lineage lineage, Pickle pickle);

    /**
     * Collect the {@link Lineage} of a
     * {@linkplain io.cucumber.messages.types.GherkinDocument GherkinDocument element}
     * or {@link Pickle} and reduce it to a single result.
     *
     * @param <T> the type reduced to.
     */
    interface Collector<T> {
        default void add(GherkinDocument document) {
    
        }
    
        default void add(Feature feature) {
    
        }
    
        default void add(Rule rule) {
    
        }
    
        default void add(Scenario scenario) {
    
        }
    
        default void add(Examples examples, int index) {
        }
    
        default void add(TableRow example, int index) {
        }
    
        default void add(Pickle pickle) {
        }
    
        T finish();
    }
}
