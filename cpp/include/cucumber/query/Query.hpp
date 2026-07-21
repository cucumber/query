#ifndef CUCUMBER_QUERY_QUERY_HPP
#define CUCUMBER_QUERY_QUERY_HPP

#include "cucumber/messages/All.hpp"
#include "cucumber/messages/Envelope.hpp"
#include "cucumber/messages/TestCaseStarted.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include <algorithm>
#include <cstddef>
#include <cucumber/messages/Feature.hpp>
#include <cucumber/messages/GherkinDocument.hpp>
#include <cucumber/messages/Pickle.hpp>
#include <cucumber/messages/Rule.hpp>
#include <cucumber/messages/TestCase.hpp>
#include <cucumber/messages/TestStepFinished.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
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

        // template<typename TFind, typename Cmp>
        // [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseFinished>> FindAllTestCaseFinishedOrderBy(TFind findOrderBy, Cmp order) const;

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

        std::optional<std::shared_ptr<const messages::Pickle>> FindPickleBy(
            std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>> element) const;

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

        std::optional<std::shared_ptr<const messages::TestCase>> FindTestCaseBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
            std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>>
                element) const;

        // FindTestCaseDurationBy(element : TestCaseStarted | TestCaseFinished)
        //     : Duration | undefined const;

        std::optional<std::shared_ptr<const messages::TestCaseStarted>> FindTestCaseStartedBy(
            std::variant<std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const;

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

        std::optional<std::shared_ptr<const messages::TestStep>> FindTestStepBy(
            std::variant<std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const;

        // FindTestStepsStartedBy(element : TestCaseStarted | TestCaseFinished)
        //     : ReadonlyArray<TestStepStarted> const;

        // FindTestStepsFinishedBy(element : TestCaseStarted | TestCaseFinished)
        //     : ReadonlyArray<TestStepFinished> const;

        std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>> FindTestStepFinishedAndTestStepBy(
            const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const;

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

        void UpdateTestCase(std::shared_ptr<const messages::TestCase> testCase);
        void UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted);
        /////////////

        void UpdateTestStepFinished(std::shared_ptr<const messages::TestStepFinished> testStepFinished);
        void UpdateTestCaseFinished(std::shared_ptr<const messages::TestCaseFinished> testCaseFinished);
        /////////////
        /////////////

        std::shared_ptr<const messages::Meta> meta;

        //   private testRunStarted: TestRunStarted
        //   private testRunFinished: TestRunFinished
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseStarted>> testCaseStartedById;
        std::unordered_map<std::string, std::unique_ptr<const Lineage>> lineageById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Step>> stepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Pickle>> pickleById;
        std::unordered_map<std::string, std::shared_ptr<const messages::PickleStep>> pickleStepById;
        //   private readonly hookById: Map<string, Hook> = new Map()
        std::unordered_map<std::string, std::shared_ptr<const messages::StepDefinition>> stepDefinitionById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCase>> testCaseById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestStep>> testStepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseFinished>> testCaseFinishedByTestCaseStartedId;
        //   private readonly testRunHookStartedById: std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookStarted> = new Map()
        //   private readonly testRunHookFinishedByTestRunHookStartedId: std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookFinished> =
        //     new Map()
        //   private readonly testStepStartedByTestCaseStartedId: ArrayMultistd::unordered_map<std::string, std::shared_ptr<const messages::TestStepStarted> =
        //     new ArrayMultimap()
        std::unordered_map<std::string, std::vector<std::shared_ptr<const messages::TestStepFinished>>> testStepFinishedByTestCaseStartedId;
        //     new ArrayMultimap()
        //   private readonly attachmentsByTestCaseStartedId: ArrayMultistd::unordered_map<std::string, std::shared_ptr<const messages::Attachment> =
        //     new ArrayMultimap()
        //   private readonly attachmentsByTestRunHookStartedId: ArrayMultistd::unordered_map<std::string, std::shared_ptr<const messages::Attachment> =
        //     new ArrayMultimap()
        //   private readonly suggestionsByPickleStepId: ArrayMultistd::unordered_map<std::string, std::shared_ptr<const messages::Suggestion> =
        //     new ArrayMultimap()
        //   private readonly undefinedParameterTypes: UndefinedParameterType[] = []
    };

    template<typename Transform, typename Cmp>
    [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseStarted>> Query::FindAllTestCaseStartedOrderBy(Transform findOrderBy, Cmp order) const
    {
        using TransformResult = decltype(findOrderBy(std::declval<const Query&>(), std::declval<std::shared_ptr<const messages::TestCaseStarted>>()));

        const auto allTestCaseStarted = FindAllTestCaseStarted();

        std::vector<std::pair<std::shared_ptr<const messages::TestCaseStarted>, TransformResult>> transformed;
        transformed.reserve(allTestCaseStarted.size());

        auto withOrderBy = std::transform(allTestCaseStarted.begin(), allTestCaseStarted.end(), std::back_inserter(transformed),
            [&](const auto& testCaseStarted) -> std::pair<std::shared_ptr<const messages::TestCaseStarted>, TransformResult>
            {
                return { testCaseStarted, findOrderBy(*this, testCaseStarted) };
            });

        std::sort(transformed.begin(), transformed.end(),
            [&](const auto& lhs, const auto& rhs)
            {
                if (!lhs.second.has_value() && !rhs.second.has_value())
                {
                    return false;
                }
                if (!lhs.second.has_value())
                {
                    return true;
                }
                if (!rhs.second.has_value())
                {
                    return false;
                }

                return order(lhs.second.value(), rhs.second.value()) < 0;
            });

        std::vector<std::shared_ptr<const messages::TestCaseStarted>> result;
        result.reserve(allTestCaseStarted.size());
        std::transform(transformed.begin(), transformed.end(), std::back_inserter(result),
            [](const auto& pair)
            {
                return pair.first;
            });

        return result;
    }
}

#endif
