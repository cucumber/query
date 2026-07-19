#ifndef CUCUMBER_QUERY_QUERY_HPP
#define CUCUMBER_QUERY_QUERY_HPP

#include "cucumber/messages/All.hpp"
#include "cucumber/messages/Envelope.hpp"
#include "cucumber/messages/TestCaseStarted.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include <cstddef>
#include <cucumber/messages/Feature.hpp>
#include <cucumber/messages/GherkinDocument.hpp>
#include <cucumber/messages/Pickle.hpp>
#include <cucumber/messages/Rule.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cucumber::query
{
    struct Lineage
    {
        std::shared_ptr<const messages::GherkinDocument> gherkinDocument;
        std::shared_ptr<const messages::Feature> feature;
        std::shared_ptr<const messages::Background> background;
        std::shared_ptr<const messages::Rule> rule;
        std::shared_ptr<const messages::Background> ruleBackground;
        std::shared_ptr<const messages::Scenario> scenario;
        std::shared_ptr<const messages::Examples> examples;
        std::size_t examplesIndex;
        std::shared_ptr<const messages::TableRow> example;
        std::size_t exampleIndex;

        Lineage operator+(const Lineage& other) const;
    };

    class Query
    {
    public:
        void Update(const cucumber::messages::Envelope& envelope);

        [[nodiscard]] std::unordered_map<messages::TestStepResultStatus, std::size_t> CountMostSevereTestStepResultStatus() const;

        [[nodiscard]] std::size_t CountTestCasesStarted() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::Pickle>> FindAllPickles() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::PickleStep>> FindAllPickleSteps() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::StepDefinition>> FindAllStepDefinitions() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseStarted>> FindAllTestCaseStarted() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseFinished>> FindAllTestCaseFinished() const;

        template<typename TFind, typename Cmp>
        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseStarted>> FindAllTestCaseStartedOrderBy(TFind findOrderBy, Cmp order) const;

        template<typename TFind, typename Cmp>
        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseFinished>> FindAllTestCaseFinishedOrderBy(TFind findOrderBy, Cmp order) const;

        // FindAllTestSteps()
        //     : ReadonlyArray<TestStep> const;

        // FindAllTestStepStarted()
        //     : ReadonlyArray<TestStepStarted> const;

        // FindAllTestStepFinished()
        //     : ReadonlyArray<TestStepFinished> const;

        // FindAllTestRunHookStarted()
        //     : ReadonlyArray<TestRunHookStarted> const;

        // FindAllTestRunHookFinished()
        //     : ReadonlyArray<TestRunHookFinished> const;

        // FindAllUndefinedParameterTypes()
        //     : ReadonlyArray<UndefinedParameterType> const;

        // FindAttachmentsBy(element : TestStepFinished | TestRunHookFinished)
        //     : ReadonlyArray<Attachment> const;

        // FindHookBy(item : TestStep | TestRunHookStarted | TestRunHookFinished)
        //     : Hook | undefined const;

        // FindMeta()
        //     : Meta | undefined const;

        // FindMostSevereTestStepResultBy(element : TestCaseStarted | TestCaseFinished)
        //     : TestStepResult | undefined const;

        // FindLocationOf(pickle : Pickle)
        //     : Location | undefined const;

        // FindPickleBy(element : TestCaseStarted | TestCaseFinished | TestStepStarted)
        //     : Pickle | undefined const;

        // FindPickleStepBy(testStep : TestStep)
        //     : PickleStep | undefined const;

        // FindStepBy(pickleStep : PickleStep)
        //     : Step | undefined const;

        // FindStepDefinitionsBy(testStep : TestStep)
        //     : ReadonlyArray<StepDefinition> const;

        // findSuggestionsBy(element : PickleStep | Pickle)
        //     : ReadonlyArray<Suggestion> const;

        // FindUnambiguousStepDefinitionBy(testStep : TestStep)
        //     : StepDefinition | undefined const;

        // FindTestCaseBy(element : TestCaseStarted | TestCaseFinished | TestStepStarted | TestStepFinished)
        //     : TestCase | undefined const;

        // FindTestCaseDurationBy(element : TestCaseStarted | TestCaseFinished)
        //     : Duration | undefined const;

        // FindTestCaseStartedBy(element : TestCaseFinished | TestStepStarted | TestStepFinished)
        //     : TestCaseStarted | undefined const;

        // FindTestCaseFinishedBy(testCaseStarted : TestCaseStarted)
        //     : TestCaseFinished | undefined const;

        // FindTestRunHookStartedBy(testRunHookFinished : TestRunHookFinished)
        //     : TestRunHookStarted | undefined const;

        // FindTestRunHookFinishedBy(testRunHookStarted : TestRunHookStarted)
        //     : TestRunHookFinished | undefined const;

        // FindTestRunDuration()
        //     : Duration | undefined const;

        // FindTestRunFinished()
        //     : TestRunFinished | undefined const;

        // FindTestRunStarted()
        //     : TestRunStarted | undefined const;

        // FindTestStepBy(element : TestStepStarted | TestStepFinished)
        //     : TestStep | undefined
        // {}

        // FindTestStepsStartedBy(element : TestCaseStarted | TestCaseFinished)
        //     : ReadonlyArray<TestStepStarted> const;

        // FindTestStepsFinishedBy(element : TestCaseStarted | TestCaseFinished)
        //     : ReadonlyArray<TestStepFinished> const;

        // FindTestStepFinishedAndTestStepBy(testCaseStarted : TestCaseStarted)
        //     : ReadonlyArray<[ TestStepFinished, TestStep ]> const;

        // FindLineageBy(element : Pickle | TestCaseStarted | TestCaseFinished)
        //     : Lineage | undefined const;

        // [[nodiscard]] std::size_t CountTestCasesStarted() const;

    private:
        void UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument);
        void UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, std::unique_ptr<Lineage> lineage);
        void UpdateRule(const std::shared_ptr<const messages::Rule>& rule, std::unique_ptr<Lineage> lineage);
        void UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, std::unique_ptr<Lineage> lineage);
        void UpdateSteps(const std::vector<std::shared_ptr<messages::Step>>& steps);
        void UpdatePickle(std::shared_ptr<const messages::Pickle> pickle);

        /////////////
        /////////////
        /////////////

        void UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted);

        /////////////
        /////////////
        /////////////

        std::shared_ptr<const messages::Meta> meta;

        //   private testRunStarted: TestRunStarted
        //   private testRunFinished: TestRunFinished
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseStarted>> testCaseStartedById;
        std::unordered_map<std::string, std::unique_ptr<const Lineage>> lineageById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Step>> stepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Pickle>> pickleById;
        //   private readonly pickleStepById: Map<string, PickleStep> = new Map()
        //   private readonly hookById: Map<string, Hook> = new Map()
        //   private readonly stepDefinitionById: Map<string, StepDefinition> = new Map()
        //   private readonly testCaseById: Map<string, TestCase> = new Map()
        //   private readonly testStepById: Map<string, TestStep> = new Map()
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseFinished>> testCaseFinishedByTestCaseStartedId;
        //   private readonly testRunHookStartedById: Map<string, TestRunHookStarted> = new Map()
        //   private readonly testRunHookFinishedByTestRunHookStartedId: Map<string, TestRunHookFinished> =
        //     new Map()
        //   private readonly testStepStartedByTestCaseStartedId: ArrayMultimap<string, TestStepStarted> =
        //     new ArrayMultimap()
        //   private readonly testStepFinishedByTestCaseStartedId: ArrayMultimap<string, TestStepFinished> =
        //     new ArrayMultimap()
        //   private readonly attachmentsByTestCaseStartedId: ArrayMultimap<string, Attachment> =
        //     new ArrayMultimap()
        //   private readonly attachmentsByTestRunHookStartedId: ArrayMultimap<string, Attachment> =
        //     new ArrayMultimap()
        //   private readonly suggestionsByPickleStepId: ArrayMultimap<string, Suggestion> =
        //     new ArrayMultimap()
        //   private readonly undefinedParameterTypes: UndefinedParameterType[] = []
    };
}

#endif
