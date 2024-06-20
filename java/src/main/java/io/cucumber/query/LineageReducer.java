package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;

public interface LineageReducer<T> {
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
