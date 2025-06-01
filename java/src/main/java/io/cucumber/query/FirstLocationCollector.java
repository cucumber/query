package io.cucumber.query;

import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.Location;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;
import io.cucumber.query.LineageReducer.Collector;

import java.util.function.Supplier;

/**
 * Finds the first (depending on
 * {@link LineageReducer#descending(Supplier)} or
 * {@link LineageReducer#ascending(Supplier)}) location of an element in
 * a lineage.
 *
 * @see NamingStrategy
 */
class FirstLocationCollector implements Collector<Location> {

    private Location location;

    @Override
    public void add(Feature feature) {
        if (location == null) {
            location = feature.getLocation();
        }
    }

    @Override
    public void add(Rule rule) {
        if (location == null) {
            location = rule.getLocation();
        }
    }

    @Override
    public void add(Scenario scenario) {
        if (location == null) {
            location = scenario.getLocation();
        }
    }

    @Override
    public void add(Examples examples, int index) {
        if (location == null) {
            location = examples.getLocation();
        }
    }

    @Override
    public void add(TableRow example, int index) {
        if (location == null) {
            location = example.getLocation();
        }
    }

    @Override
    public Location finish() {
        return location;
    }
}
