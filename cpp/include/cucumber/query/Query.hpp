#ifndef CUCUMBER_QUERY_QUERY_HPP
#define CUCUMBER_QUERY_QUERY_HPP

#include "cucumber/messages/All.hpp"
#include "cucumber/messages/Envelope.hpp"
#include "cucumber/messages/Feature.hpp"
#include "cucumber/messages/GherkinDocument.hpp"
#include "cucumber/messages/Pickle.hpp"
#include "cucumber/messages/Rule.hpp"
#include "cucumber/messages/TestCase.hpp"
#include "cucumber/messages/TestCaseStarted.hpp"
#include "cucumber/messages/TestStepFinished.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
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

        template<typename TFind, typename Cmp>
        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseFinished>> FindAllTestCaseFinishedOrderBy(TFind findOrderBy, Cmp order) const;

        // std::vector<std::shared_ptr<messages::TestStep>> FindAllTestSteps()
        //     ;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCase>> FindAllTestCases() const;

        // std::vector<std::shared_ptr<messages::TestStepStarted>> FindAllTestStepStarted()
        //     ;

        // std::vector<std::shared_ptr<messages::TestStepFinished>> FindAllTestStepFinished()
        //     ;

        // std::vector<std::shared_ptr<messages::TestRunHookStarted>> FindAllTestRunHookStarted()
        //     ;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestRunHookFinished>> FindAllTestRunHookFinished() const;

        // std::vector<std::shared_ptr<messages::UndefinedParameterType>> FindAllUndefinedParameterTypes()
        //     ;

        // std::vector<std::shared_ptr<messages::Attachment>> FindAttachmentsBy(element : TestStepFinished | TestRunHookFinished)
        //     ;

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

        // std::vector<std::shared_ptr<messages::StepDefinition>> FindStepDefinitionsBy(testStep : TestStep)
        //     ;

        // std::vector<std::shared_ptr<messages::Suggestion>> findSuggestionsBy(element : PickleStep | Pickle)
        //     ;

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

        // std::vector<std::shared_ptr<messages::TestStepStarted>> FindTestStepsStartedBy(element : TestCaseStarted | TestCaseFinished)
        //     ;

        // std::vector<std::shared_ptr<messages::TestStepFinished>> FindTestStepsFinishedBy(element : TestCaseStarted | TestCaseFinished)
        //     ;

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
        void UpdateTestRunHookStarted(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted);
        void UpdateTestRunHookFinished(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished);

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
        std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookStarted>> testRunHookStartedById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookFinished>> testRunHookFinishedByTestRunHookStartedId;
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

    namespace detail
    {
        template<typename TElement, typename Transform, typename Cmp>
        [[nodiscard]] std::vector<std::shared_ptr<const TElement>> FindAllOrderBy(const Query& query, const std::vector<std::shared_ptr<const TElement>>& allElements, Transform findOrderBy, Cmp order)
        {
            using TransformResult = decltype(std::invoke(findOrderBy, query, std::declval<std::shared_ptr<const TElement>>()));

            std::vector<std::pair<std::shared_ptr<const TElement>, TransformResult>> transformed;
            transformed.reserve(allElements.size());

            for (const auto& element : allElements)
            {
                transformed.emplace_back(element, std::invoke(findOrderBy, query, element));
            }

            std::sort(transformed.begin(), transformed.end(),
                [&](const auto& lhs, const auto& rhs)
                {
                    const auto lhsHasValue = lhs.second.has_value();
                    const auto rhsHasValue = rhs.second.has_value();
                    if (!lhsHasValue && !rhsHasValue)
                    {
                        return false;
                    }
                    if (!lhsHasValue)
                    {
                        return true;
                    }
                    if (!rhsHasValue)
                    {
                        return false;
                    }

                    return std::invoke(order, lhs.second.value(), rhs.second.value()) < 0;
                });

            std::vector<std::shared_ptr<const TElement>> result;
            result.reserve(allElements.size());
            for (const auto& pair : transformed)
            {
                result.push_back(pair.first);
            }

            return result;
        }
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseStarted>> Query::FindAllTestCaseStartedOrderBy(Transform findOrderBy, Cmp order) const
    {
        return detail::FindAllOrderBy<messages::TestCaseStarted>(*this, FindAllTestCaseStarted(), std::move(findOrderBy), std::move(order));
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCaseFinished>> Query::FindAllTestCaseFinishedOrderBy(Transform findOrderBy, Cmp order) const
    {
        return detail::FindAllOrderBy<messages::TestCaseFinished>(*this, FindAllTestCaseFinished(), std::move(findOrderBy), std::move(order));
    }
}

#endif
