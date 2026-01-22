package io.cucumber.query;

import org.jspecify.annotations.Nullable;

record OrderableMessage<M, T>(M message, @Nullable T orderBy) {

    OrderableMessage(M message) {
        this(message, null);
    }
}
