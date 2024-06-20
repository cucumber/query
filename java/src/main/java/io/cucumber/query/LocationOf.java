package io.cucumber.query;

import io.cucumber.messages.types.Location;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;

class LocationOf implements LineageReducer<Location> {
    private Location location;

    @Override
    public void add(Scenario scenario) {
        location = scenario.getLocation();
    }

    @Override
    public void add(TableRow example, int index) {
        location = example.getLocation();
    }

    @Override
    public Location finish() {
        return location;
    }
}
