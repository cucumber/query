package io.cucumber.query;

import org.jspecify.annotations.Nullable;

final class OrderableMessage<M, T> {
    private final M message;
    private final @Nullable T orderBy;

    OrderableMessage(M message, @Nullable T orderBy) {
        this.message = message;
        this.orderBy = orderBy;
    }

    OrderableMessage(M message) {
        this.message = message;
        this.orderBy = null;
    }

    M getMessage() {
        return message;
    }

    @Nullable T getOrderBy() {
        return orderBy;
    }
}
