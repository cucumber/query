package io.cucumber.query;

final class OrderableMessage<M, T> {
    private final M message;
    private final T orderBy;

    OrderableMessage(M message, T orderBy) {
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

    T getOrderBy() {
        return orderBy;
    }
}
