package io.cucumber.query;

import io.cucumber.messages.NdjsonToMessageIterable;
import io.cucumber.messages.types.Background;
import io.cucumber.messages.types.Envelope;
import io.cucumber.messages.types.Examples;
import io.cucumber.messages.types.Feature;
import io.cucumber.messages.types.GherkinDocument;
import io.cucumber.messages.types.Pickle;
import io.cucumber.messages.types.Rule;
import io.cucumber.messages.types.Scenario;
import io.cucumber.messages.types.TableRow;
import org.jspecify.annotations.NonNull;
import org.junit.jupiter.api.Test;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import static io.cucumber.query.Repository.RepositoryFeature.INCLUDE_GHERKIN_DOCUMENTS;
import static org.assertj.core.api.Assertions.assertThat;

class LineageTest {

    final Repository repository = Repository.builder()
            .feature(INCLUDE_GHERKIN_DOCUMENTS, true)
            .build();
    final Query query = new Query(repository);

    @Test
    void minimal() throws IOException {
        List<Envelope> messages = readMessages(Paths.get("../testdata/src/minimal.ndjson"));
        messages.forEach(repository::update);
        Pickle pickle = query.findAllPickles().stream()
                .findFirst()
                .get();
        Lineage lineage = query.findLineageBy(pickle).get();

        GherkinDocument gherkinDocument = messages.stream().filter(envelope -> envelope.getGherkinDocument().isPresent())
                .map(Envelope::getGherkinDocument)
                .map(Optional::get)
                .findFirst()
                .get();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Scenario> scenario = feature.get().getChildren().get(0).getScenario();

        assertThat(lineage.document()).isEqualTo(gherkinDocument);
        assertThat(lineage.feature()).isEqualTo(feature);
        assertThat(lineage.background()).isEmpty();
        assertThat(lineage.rule()).isEmpty();
        assertThat(lineage.ruleBackground()).isEmpty();
        assertThat(lineage.scenario()).isEqualTo(scenario);
        assertThat(lineage.examples()).isEmpty();
        assertThat(lineage.example()).isEmpty();
    }

    @Test
    void exampleTables() throws IOException {
        List<Envelope> messages = readMessages(Paths.get("../testdata/src/examples-tables.ndjson"));
        messages.forEach(repository::update);
        Pickle pickle = query.findAllPickles().stream()
                .findFirst()
                .get();
        Lineage lineage = query.findLineageBy(pickle).get();

        GherkinDocument gherkinDocument = messages.stream().filter(envelope -> envelope.getGherkinDocument().isPresent())
                .map(Envelope::getGherkinDocument)
                .map(Optional::get)
                .findFirst()
                .get();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Scenario> scenario = feature.get().getChildren().get(0).getScenario();
        Examples examples = scenario.get().getExamples().get(0);
        TableRow example = examples.getTableBody().get(0);

        assertThat(lineage.document()).isEqualTo(gherkinDocument);
        assertThat(lineage.feature()).isEqualTo(feature);
        assertThat(lineage.background()).isEmpty();
        assertThat(lineage.rule()).isEmpty();
        assertThat(lineage.ruleBackground()).isEmpty();
        assertThat(lineage.scenario()).isEqualTo(scenario);
        assertThat(lineage.examples()).contains(examples);
        assertThat(lineage.example()).contains(example);
    }

    @Test
    void rulesBackgrounds() throws IOException {
        List<Envelope> messages = readMessages(Paths.get("../testdata/src/rules-backgrounds.ndjson"));
        messages.forEach(repository::update);
        Pickle pickle = query.findAllPickles().stream()
                .findFirst()
                .get();
        Lineage lineage = query.findLineageBy(pickle).get();

        GherkinDocument gherkinDocument = messages.stream().filter(envelope -> envelope.getGherkinDocument().isPresent())
                .map(Envelope::getGherkinDocument)
                .map(Optional::get)
                .findFirst()
                .get();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Background> background = feature.get().getChildren().get(0).getBackground();
        Optional<Rule> rule = feature.get().getChildren().get(1).getRule();
        Optional<Background> ruleBackGround = rule.get().getChildren().get(0).getBackground();
        Optional<Scenario> scenario = rule.get().getChildren().get(1).getScenario();

        assertThat(lineage.document()).isEqualTo(gherkinDocument);
        assertThat(lineage.feature()).isEqualTo(feature);
        assertThat(lineage.background()).isEqualTo(background);
        assertThat(lineage.rule()).isEqualTo(rule);
        assertThat(lineage.ruleBackground()).isEqualTo(ruleBackGround);
        assertThat(lineage.scenario()).isEqualTo(scenario);
        assertThat(lineage.examples()).isEmpty();
        assertThat(lineage.example()).isEmpty();
    }

    private static @NonNull List<Envelope> readMessages(Path path) throws IOException {
        InputStream in = Files.newInputStream(path);
        NdjsonToMessageIterable messages = new NdjsonToMessageIterable(in, json -> Jackson.OBJECT_MAPPER.readValue(json, Envelope.class));
        List<Envelope> e = new ArrayList<>();
        messages.forEach(e::add);
        return e;
    }
}
