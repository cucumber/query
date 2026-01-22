package io.cucumber.query;

import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.TestCaseFinished;
import io.cucumber.messages.types.TestCaseStarted;
import io.cucumber.messages.types.Timestamp;
import org.junit.jupiter.api.Test;

import java.util.UUID;
import java.util.stream.Stream;

import static org.assertj.core.api.Assertions.assertThat;

class QueryTest {

    final Repository repository = Repository.builder().build();
    final Query query = new Query(repository);

    @Test
    void retainsInsertionOrderForTestCaseStarted() {
        TestCaseStarted a = new TestCaseStarted(0L, randomId(), randomId(), "main", new Timestamp(1L, 0));
        TestCaseStarted b = new TestCaseStarted(0L, randomId(), randomId(), "main", new Timestamp(1L, 0));
        TestCaseStarted c = new TestCaseStarted(0L, randomId(), randomId(), "main", new Timestamp(1L, 0));

        Stream.of(a, b, c)
                .map(Envelope::of)
                .forEach(repository::update);

        assertThat(query.findAllTestCaseStarted()).containsExactly(a, b, c);
    }

    @Test
    void omitsTestCaseStartedIfFinishedAndWillBeRetried() {
        TestCaseStarted a = new TestCaseStarted(0L, randomId(), randomId(), "main", new Timestamp(0L, 0));
        TestCaseFinished b = new TestCaseFinished(a.getId(), new Timestamp(0L, 0), true);
        TestCaseStarted c = new TestCaseStarted(0L, randomId(), randomId(), "main", new Timestamp(0L, 0));
        TestCaseFinished d = new TestCaseFinished(c.getId(), new Timestamp(0L, 0), false);

        Stream.of(a, c)
                .map(Envelope::of)
                .forEach(repository::update);
        Stream.of(b, d)
                .map(Envelope::of)
                .forEach(repository::update);

        assertThat(query.findAllTestCaseStarted()).containsExactly(c);
        assertThat(query.countTestCasesStarted()).isEqualTo(1);
    }

    private static String randomId() {
        return UUID.randomUUID().toString();
    }

}
