package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;

import java.util.function.Supplier;

import static java.util.Objects.requireNonNull;

/**
 * TODO: Explain how this solves the lack of an AST that has nodes.
 *
 * @param <T>
 */
abstract class LineageVisitingStrategy<T> {

    abstract T visit(Lineage elements, Pickle pickle);

    private static <T> void visitExampleIndex(CollectingVisitor<T> visitor, Lineage elements, Integer index) {
        Integer examplesIndex = elements.examplesIndex().orElse(0);
        visitor.accept(examplesIndex, index);
    }

    static class All<T> extends LineageVisitingStrategy<T> {

        private final Supplier<CollectingVisitor<T>> visitorSupplier;

        public All(Supplier<CollectingVisitor<T>> visitorSupplier) {
            this.visitorSupplier = requireNonNull(visitorSupplier);
        }

        @Override
        T visit(Lineage elements, Pickle pickle) {
            CollectingVisitor<T> visitor = visitorSupplier.get();
            elements.feature().ifPresent(visitor::accept);
            elements.rule().ifPresent(visitor::accept);
            elements.scenario().ifPresent(visitor::accept);
            elements.examples().ifPresent(visitor::accept);
            elements.exampleIndex().ifPresent(index -> visitExampleIndex(visitor, elements, index));
            visitor.accept(pickle);
            return visitor.collect();
        }
    }

    static class Parent<T> extends LineageVisitingStrategy<T> {

        private final Supplier<CollectingVisitor<T>> visitorSupplier;

        public Parent(Supplier<CollectingVisitor<T>> visitorSupplier) {
            this.visitorSupplier = requireNonNull(visitorSupplier);
        }

        @Override
        T visit(Lineage elements, Pickle pickle) {
            CollectingVisitor<T> visitor = visitorSupplier.get();
            if (elements.exampleIndex().isPresent()) {
                elements.exampleIndex().ifPresent(index -> visitExampleIndex(visitor, elements, index));
            } else if (elements.examples().isPresent()) {
                elements.examples().ifPresent(visitor::accept);
            } else if (elements.scenario().isPresent()) {
                elements.scenario().ifPresent(visitor::accept);
            } else if (elements.rule().isPresent()) {
                elements.rule().ifPresent(visitor::accept);
            } else if (elements.feature().isPresent()) {
                elements.feature().ifPresent(visitor::accept);
            }
            visitor.accept(pickle);
            return visitor.collect();
        }
    }

    public interface CollectingVisitor<T> {
        default void accept(Feature feature) {

        }

        default void accept(Rule rule) {

        }

        default void accept(Scenario scenario) {

        }

        default void accept(Examples examples) {
        }

        default void accept(int examplesIndex, int exampleIndex) {
        }

        default void accept(Pickle pickle) {
        }

        T collect();
    }

}
