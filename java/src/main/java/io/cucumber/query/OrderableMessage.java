package io.cucumber.query;

import java.util.Comparator;

import static java.util.Comparator.naturalOrder;
import static java.util.Comparator.nullsFirst;
import static java.util.Objects.requireNonNull;

final class OrderableMessage<T> implements Comparable<OrderableMessage<T>> {
    private final T message;
    private final String uri;
    private final Long line;

    OrderableMessage(T message) {
        this(message, null,null);
    }
    
    OrderableMessage(T message, String uri, Long line) {
        this.message = requireNonNull(message);
        this.uri = uri;
        this.line = line;
    }

    private final Comparator<OrderableMessage<T>> comparator = Comparator
            .comparing((OrderableMessage<T> ord) -> ord.uri, nullsFirst(naturalOrder()))
            .thenComparing(ord -> ord.line, nullsFirst(naturalOrder()));


    @Override
    public int compareTo(OrderableMessage<T> o) {
        return comparator.compare(this, o);
    }

    T getMessage() {
        return message;
    }
}
