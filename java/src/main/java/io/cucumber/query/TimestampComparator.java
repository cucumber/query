package io.cucumber.query;

import io.cucumber.messages.types.Timestamp;

import java.util.Comparator;

class TimestampComparator implements Comparator<Timestamp> {
    @Override
    public int compare(Timestamp a, Timestamp b) {
        long sa = a.getSeconds();
        long sb = b.getSeconds();

        if (sa < sb) {
            return -1;
        } else if (sb < sa) {
            return 1;
        }

        long na = a.getNanos();
        long nb = b.getNanos();

        if (na < nb) {
            return -1;
        } else if (nb < na) {
            return 1;
        }

        return 0;
    }
}
