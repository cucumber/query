package io.cucumber.query;

import io.cucumber.messages.NdjsonToMessageReader;
import io.cucumber.messages.ndjson.Deserializer;
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
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
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
                .orElseThrow();
        Lineage lineage = query.findLineageBy(pickle).orElseThrow();

        GherkinDocument gherkinDocument = messages.stream()
                .map(Envelope::getGherkinDocument)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .findFirst()
                .orElseThrow();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Scenario> scenario = feature.orElseThrow().getChildren().get(0).getScenario();

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
                .orElseThrow();
        Lineage lineage = query.findLineageBy(pickle).orElseThrow();

        GherkinDocument gherkinDocument = messages.stream()
                .map(Envelope::getGherkinDocument)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .findFirst()
                .orElseThrow();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Scenario> scenario = feature.orElseThrow().getChildren().get(0).getScenario();
        Examples examples = scenario.orElseThrow().getExamples().get(0);
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
                .orElseThrow();
        Lineage lineage = query.findLineageBy(pickle).orElseThrow();

        GherkinDocument gherkinDocument = messages.stream()
                .map(Envelope::getGherkinDocument)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .findFirst()
                .orElseThrow();
        Optional<Feature> feature = gherkinDocument.getFeature();
        Optional<Background> background = feature.orElseThrow().getChildren().get(0).getBackground();
        Optional<Rule> rule = feature.get().getChildren().get(1).getRule();
        Optional<Background> ruleBackGround = rule.orElseThrow().getChildren().get(0).getBackground();
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
        var in = Files.newInputStream(path);
        var reader = new NdjsonToMessageReader(in, new Deserializer());
        return reader.lines().toList();
    }
}
