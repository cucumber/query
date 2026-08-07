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
        auto Update(const cucumber::messages::Envelope& envelope) -> void;

        [[nodiscard]] auto CountMostSevereTestStepResultStatus() const -> std::unordered_map<messages::TestStepResultStatus, std::size_t>;

        [[nodiscard]] auto CountTestCasesStarted() const -> std::size_t;

        [[nodiscard]] auto FindAllPickles() const -> std::vector<std::shared_ptr<const messages::Pickle>>;

        [[nodiscard]] auto FindAllPickleSteps() const -> std::vector<std::shared_ptr<const messages::PickleStep>>;

        [[nodiscard]] auto FindAllStepDefinitions() const -> std::vector<std::shared_ptr<const messages::StepDefinition>>;

        [[nodiscard]] auto FindAllTestCaseStarted() const -> std::vector<std::shared_ptr<const messages::TestCaseStarted>>;

        [[nodiscard]] auto FindAllTestCaseFinished() const -> std::vector<std::shared_ptr<const messages::TestCaseFinished>>;

        template<typename TFind, typename Cmp>
        [[nodiscard]] auto FindAllTestCaseStartedOrderBy(TFind findOrderBy, Cmp order) const -> std::vector<std::shared_ptr<const messages::TestCaseStarted>>;

        template<typename TFind, typename Cmp>
        [[nodiscard]] auto FindAllTestCaseFinishedOrderBy(TFind findOrderBy, Cmp order) const -> std::vector<std::shared_ptr<const messages::TestCaseFinished>>;

        [[nodiscard]] auto FindAllTestSteps() const -> std::vector<std::shared_ptr<const messages::TestStep>>;

        [[nodiscard]] auto FindAllTestCases() const -> std::vector<std::shared_ptr<const messages::TestCase>>;

        [[nodiscard]] auto FindAllTestStepStarted() const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>;

        [[nodiscard]] auto FindAllTestStepFinished() const -> std::vector<std::shared_ptr<const messages::TestStepFinished>>;

        [[nodiscard]] auto FindAllTestRunHookStarted() const -> std::vector<std::shared_ptr<const messages::TestRunHookStarted>>;

        [[nodiscard]] auto FindAllTestRunHookFinished() const -> std::vector<std::shared_ptr<const messages::TestRunHookFinished>>;

        [[nodiscard]] auto FindAllUndefinedParameterTypes() const -> std::vector<std::shared_ptr<const messages::UndefinedParameterType>>;

        [[nodiscard]] auto FindAttachmentsBy(std::variant<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestRunHookFinished>> element) const
            -> std::vector<std::shared_ptr<const messages::Attachment>>;

        [[nodiscard]] auto FindHookBy(
            std::variant<std::shared_ptr<const messages::TestStep>, std::shared_ptr<const messages::TestRunHookStarted>, std::shared_ptr<const messages::TestRunHookFinished>> element) const
            -> std::optional<std::shared_ptr<const messages::Hook>>;

        [[nodiscard]] auto FindMeta() const -> std::optional<std::shared_ptr<const messages::Meta>>;

        [[nodiscard]] auto FindMostSevereTestStepResultBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
            -> std::optional<std::shared_ptr<const messages::TestStepResult>>;

        [[nodiscard]] auto FindLocationOf(const std::shared_ptr<const messages::Pickle>& pickle) const -> std::optional<std::shared_ptr<const messages::Location>>;

        [[nodiscard]] auto FindPickleBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
            std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const -> std::optional<std::shared_ptr<const messages::Pickle>>;

        [[nodiscard]] auto FindPickleStepBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::optional<std::shared_ptr<const messages::PickleStep>>;

        [[nodiscard]] auto FindStepBy(const std::shared_ptr<const messages::PickleStep>& pickleStep) const -> std::optional<std::shared_ptr<const messages::Step>>;

        [[nodiscard]] auto FindStepDefinitionsBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::vector<std::shared_ptr<const messages::StepDefinition>>;

        [[nodiscard]] auto FindSuggestionsBy(const std::shared_ptr<const messages::PickleStep>& element) const -> std::vector<std::shared_ptr<const messages::Suggestion>>;
        [[nodiscard]] auto FindSuggestionsBy(const std::shared_ptr<const messages::Pickle>& element) const -> std::vector<std::shared_ptr<const messages::Suggestion>>;

        [[nodiscard]] auto FindUnambiguousStepDefinitionBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::optional<std::shared_ptr<const messages::StepDefinition>>;

        [[nodiscard]] auto FindTestCaseBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
            std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const -> std::optional<std::shared_ptr<const messages::TestCase>>;

        [[nodiscard]] auto FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<std::shared_ptr<const messages::Duration>>;

        [[nodiscard]] auto FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::Duration>>;

        [[nodiscard]] auto FindTestCaseStartedBy(
            std::variant<std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const
            -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>;

        [[nodiscard]] auto FindTestCaseFinishedBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const -> std::optional<std::shared_ptr<const messages::TestCaseFinished>>;

        [[nodiscard]] auto FindTestRunHookStartedBy(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) const
            -> std::optional<std::shared_ptr<const messages::TestRunHookStarted>>;

        [[nodiscard]] auto FindTestRunHookFinishedBy(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted) const
            -> std::optional<std::shared_ptr<const messages::TestRunHookFinished>>;

        [[nodiscard]] auto FindTestRunDuration() const -> std::optional<std::shared_ptr<const messages::Duration>>;

        [[nodiscard]] auto FindTestRunFinished() const -> std::optional<std::shared_ptr<const messages::TestRunFinished>>;

        [[nodiscard]] auto FindTestRunStarted() const -> std::optional<std::shared_ptr<const messages::TestRunStarted>>;

        [[nodiscard]] auto FindTestStepBy(std::variant<std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const
            -> std::optional<std::shared_ptr<const messages::TestStep>>;

        [[nodiscard]] auto FindTestStepsStartedBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>;
        [[nodiscard]] auto FindTestStepsStartedBy(const std::shared_ptr<const messages::TestCaseFinished>& testCaseFinished) const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>;

        [[nodiscard]] auto FindTestStepsFinishedBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
            -> std::vector<std::shared_ptr<const messages::TestStepFinished>>;

        [[nodiscard]] auto FindTestStepFinishedAndTestStepBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const
            -> std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>>;

        [[nodiscard]] auto FindLineageBy(
            std::variant<std::shared_ptr<const messages::Pickle>, std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
            -> std::optional<LineageAndPickle>;

    private:
        auto UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument) -> void;
        auto UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, const std::shared_ptr<Lineage>& lineage) -> void;
        auto UpdateRule(const std::shared_ptr<const messages::Rule>& rule, const std::shared_ptr<Lineage>& lineage) -> void;
        auto UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, const std::shared_ptr<Lineage>& lineage) -> void;
        auto UpdateSteps(const std::vector<std::shared_ptr<messages::Step>>& steps) -> void;
        auto UpdatePickle(std::shared_ptr<const messages::Pickle> pickle) -> void;
        auto UpdateTestRunHookStarted(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted) -> void;
        auto UpdateTestRunHookFinished(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) -> void;
        auto UpdateTestCase(std::shared_ptr<const messages::TestCase> testCase) -> void;
        auto UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted) -> void;
        auto UpdateAttachment(const std::shared_ptr<const messages::Attachment>& attachment) -> void;
        auto UpdateTestStepFinished(std::shared_ptr<const messages::TestStepFinished> testStepFinished) -> void;
        auto UpdateTestCaseFinished(std::shared_ptr<const messages::TestCaseFinished> testCaseFinished) -> void;

        std::optional<std::shared_ptr<const messages::Meta>> meta;

        std::optional<std::shared_ptr<const messages::TestRunStarted>> testRunStarted;
        std::optional<std::shared_ptr<const messages::TestRunFinished>> testRunFinished;

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
        std::unordered_map<std::string, std::shared_ptr<const messages::Suggestion>> suggestionsByPickleStepId;
        std::vector<std::shared_ptr<const messages::UndefinedParameterType>> undefinedParameterTypes;
    };

    namespace detail
    {
        template<typename TElement, typename Transform, typename Cmp>
        [[nodiscard]] auto FindAllOrderBy(const Query& query, const std::vector<std::shared_ptr<const TElement>>& allElements, Transform findOrderBy, Cmp order)
            -> std::vector<std::shared_ptr<const TElement>>
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
    [[nodiscard]] auto Query::FindAllTestCaseStartedOrderBy(Transform findOrderBy, Cmp order) const -> std::vector<std::shared_ptr<const messages::TestCaseStarted>>
    {
        return detail::FindAllOrderBy<messages::TestCaseStarted>(*this, FindAllTestCaseStarted(), std::move(findOrderBy), std::move(order));
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] auto Query::FindAllTestCaseFinishedOrderBy(Transform findOrderBy, Cmp order) const -> std::vector<std::shared_ptr<const messages::TestCaseFinished>>
    {
        return detail::FindAllOrderBy<messages::TestCaseFinished>(*this, FindAllTestCaseFinished(), std::move(findOrderBy), std::move(order));
    }
}

#endif
