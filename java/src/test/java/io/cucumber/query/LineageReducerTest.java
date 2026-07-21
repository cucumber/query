package io.cucumber.query;

import io.cucumber.messages.NdjsonToMessageReader;
import io.cucumber.messages.ndjson.Json;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;
import io.cucumber.query.LineageReducer.Collector;
import org.junit.jupiter.api.Test;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_GHERKIN_DOCUMENTS;
import static org.assertj.core.api.Assertions.assertThat;

class LineageReducerTest {

    private static final NdjsonToMessageReader.Deserializer deserializer = Json.instance()
            .map(json -> json.deserializer(Envelope.class))
            .orElseThrow()::readValue;

    final Repository repository = Repository.builder()
            .feature(INCLUDE_GHERKIN_DOCUMENTS, true)
            .build();
    final Query query = new Query(repository);

    @Test
    void minimalDescending() throws IOException {
        var reducer = LineageReducer.descending(TestCollector::new);
        var path = Paths.get("../testdata/src/minimal.ndjson");
        var reduced = reduce(path, reducer);
        assertThat(reduced).containsExactly(
                "samples/minimal/minimal.feature",
                "minimal",
                "cukes"
        );
    }

    @Test
    void minimalAscending() throws IOException {
        var reducer = LineageReducer.ascending(TestCollector::new);
        var path = Paths.get("../testdata/src/minimal.ndjson");
        var reduced = reduce(path, reducer);
        assertThat(reduced).containsExactly(
                "cukes",
                "minimal",
                "samples/minimal/minimal.feature"
        );
    }

    @Test
    void exampleTablesDescending() throws IOException {
        var reducer = LineageReducer.descending(TestCollector::new);
        var path = Paths.get("../testdata/src/examples-tables.ndjson");
        var reduced = reduce(path, reducer);
        assertThat(reduced).containsExactly(
                "samples/examples-tables/examples-tables.feature",
                "Examples Tables",
                "Eating cucumbers",
                "These are passing #0",
                "#0"
        );
    }

    @Test
    void exampleTablesAscending() throws IOException {
        var reducer = LineageReducer.ascending(TestCollector::new);
        var path = Paths.get("../testdata/src/examples-tables.ndjson");
        var reduced = reduce(path, reducer);
        assertThat(reduced).containsExactly(
                "#0",
                "These are passing #0",
                "Eating cucumbers",
                "Examples Tables",
                "samples/examples-tables/examples-tables.feature"
        );
    }

    private List<String> reduce(Path path, LineageReducer<List<String>> reducer) throws IOException {
        readMessages(path).forEach(repository::update);
        var pickle = query.findAllPickles().stream().findFirst().orElseThrow();
        var lineage = query.findLineageBy(pickle).orElseThrow();
        return reducer.reduce(lineage);
    }

    private static List<Envelope> readMessages(Path path) throws IOException {
        var in = Files.newInputStream(path);
        var reader = new NdjsonToMessageReader(in, deserializer);
        return reader.lines().toList();
    }

    private static final class TestCollector implements Collector<List<String>> {
        private final List<String> values = new ArrayList<>();

        @Override
        public void add(GherkinDocument document) {
            document.getUri().ifPresent(values::add);
        }

        @Override
        public void add(Feature feature) {
            values.add(feature.getName());
        }

        @Override
        public void add(Rule rule) {
            values.add(rule.getName());
        }

        @Override
        public void add(Scenario scenario) {
            values.add(scenario.getName());
        }

        @Override
        public void add(Examples examples, int index) {
            values.add(examples.getName() + " #" + index);
        }

        @Override
        public void add(TableRow example, int index) {
            values.add("#" + index);
        }

        @Override
        public void add(Pickle pickle) {
            values.add(pickle.getName());
        }

        @Override
        public List<String> finish() {
            return values;
        }
    }

}
