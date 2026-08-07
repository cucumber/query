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
#include "cucumber/query/Lineage.hpp"
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
    struct LineageAndPickle
    {
        std::shared_ptr<const Lineage> lineage;
        std::shared_ptr<const messages::Pickle> pickle;
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

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestStep>> FindAllTestSteps() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestCase>> FindAllTestCases() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestStepStarted>> FindAllTestStepStarted() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestStepFinished>> FindAllTestStepFinished() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestRunHookStarted>> FindAllTestRunHookStarted() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestRunHookFinished>> FindAllTestRunHookFinished() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::UndefinedParameterType>> FindAllUndefinedParameterTypes() const;

        [[nodiscard]] std::vector<std::shared_ptr<const messages::Attachment>> FindAttachmentsBy(
            std::variant<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestRunHookFinished>> element) const;

        [[nodiscard]] std::optional<std::shared_ptr<const messages::Hook>> FindHookBy(
            std::variant<std::shared_ptr<const messages::TestStep>, std::shared_ptr<const messages::TestRunHookStarted>, std::shared_ptr<const messages::TestRunHookFinished>> element) const;

        [[nodiscard]] std::optional<std::shared_ptr<const messages::Meta>> FindMeta() const;

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::TestStepResult>> FindMostSevereTestStepResultBy(TestCaseStarted | TestCaseFinished &element) const

        [[nodiscard]] std::optional<std::shared_ptr<const messages::Location>> FindLocationOf(const std::shared_ptr<const messages::Pickle>& pickle) const;

        [[nodiscard]] std::optional<std::shared_ptr<const messages::Pickle>> FindPickleBy(
            std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>> element) const;

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::PickleStep>> FindPickleStepBy(TestStep &testStep) const

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::Step>> FindStepBy(PickleStep &pickleStep) const

        // [[nodiscard]] std::vector<std::shared_ptr<const messages::StepDefinition>> FindStepDefinitionsBy(TestStep) const &testStep

        // [[nodiscard]] std::vector<std::shared_ptr<const messages::Suggestion>> findSuggestionsBy(PickleStep | Pickle) const &element

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::StepDefinition>> FindUnambiguousStepDefinitionBy(TestStep &testStep) const

        [[nodiscard]] std::optional<std::shared_ptr<const messages::TestCase>> FindTestCaseBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>,
            std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const;

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::Duration>> FindTestCaseDurationBy(TestCaseStarted | TestCaseFinished &element) const

        [[nodiscard]] std::optional<std::shared_ptr<const messages::TestCaseStarted>> FindTestCaseStartedBy(
            std::variant<std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const;

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::TestCaseFinished>> FindTestCaseFinishedBy(TestCaseStarted &testCaseStarted) const

        [[nodiscard]] std::optional<std::shared_ptr<const messages::TestRunHookStarted>> FindTestRunHookStartedBy(
            const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) const;

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::TestRunHookFinished>> FindTestRunHookFinishedBy(TestRunHookStarted &testRunHookStarted) const

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::Duration>> FindTestRunDuration() const

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::TestRunFinished>> FindTestRunFinished() const

        // [[nodiscard]] std::optional<std::shared_ptr<const messages::TestRunStarted>> FindTestRunStarted() const

        [[nodiscard]] std::optional<std::shared_ptr<const messages::TestStep>> FindTestStepBy(
            std::variant<std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const;

        // [[nodiscard]] std::vector<std::shared_ptr<const messages::TestStepStarted>> FindTestStepsStartedBy(TestCaseStarted | TestCaseFinished) const &element

        [[nodiscard]] std::vector<std::shared_ptr<const messages::TestStepFinished>> FindTestStepsFinishedBy(
            std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const;

        [[nodiscard]] std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>> FindTestStepFinishedAndTestStepBy(
            const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const;

        [[nodiscard]] std::optional<LineageAndPickle> FindLineageBy(
            std::variant<std::shared_ptr<const messages::Pickle>, std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const;

        // [[nodiscard]] std::size_t CountTestCasesStarted() const

    private:
        void UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument);
        void UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, const std::shared_ptr<Lineage>& lineage);
        void UpdateRule(const std::shared_ptr<const messages::Rule>& rule, const std::shared_ptr<Lineage>& lineage);
        void UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, const std::shared_ptr<Lineage>& lineage);
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

        void UpdateAttachment(const std::shared_ptr<const messages::Attachment>& attachment);
        void UpdateTestStepFinished(std::shared_ptr<const messages::TestStepFinished> testStepFinished);
        void UpdateTestCaseFinished(std::shared_ptr<const messages::TestCaseFinished> testCaseFinished);
        /////////////
        /////////////

        std::optional<std::shared_ptr<const messages::Meta>> meta;

        //   private testRunStarted: TestRunStarted
        //   private testRunFinished: TestRunFinished
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseStarted>> testCaseStartedById;
        std::unordered_map<std::string, std::shared_ptr<const Lineage>> lineageById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Step>> stepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Pickle>> pickleById;
        std::unordered_map<std::string, std::shared_ptr<const messages::PickleStep>> pickleStepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::Hook>> hookById;
        std::unordered_map<std::string, std::shared_ptr<const messages::StepDefinition>> stepDefinitionById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCase>> testCaseById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestStep>> testStepById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestCaseFinished>> testCaseFinishedByTestCaseStartedId;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookStarted>> testRunHookStartedById;
        std::unordered_map<std::string, std::shared_ptr<const messages::TestRunHookFinished>> testRunHookFinishedByTestRunHookStartedId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<const messages::TestStepStarted>>> testStepStartedByTestCaseStartedId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<const messages::TestStepFinished>>> testStepFinishedByTestCaseStartedId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<const messages::Attachment>>> attachmentsByTestCaseStartedId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<const messages::Attachment>>> attachmentsByTestRunHookStartedId;
        //   std::unordered_map<std::string, std::shared_ptr<const messages::Suggestion>> suggestionsByPickleStepId;
        std::vector<std::shared_ptr<const messages::UndefinedParameterType>> undefinedParameterTypes;
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
