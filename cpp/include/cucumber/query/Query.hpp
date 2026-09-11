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
#include "cucumber/query/View.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cucumber::query
{
    struct LineageAndPickle
    {
        const Lineage* lineage;
        const messages::Pickle* pickle;
    };

    struct TestStepFinishedAndTestStep
    {
        const messages::TestStepFinished* testStepFinished;
        const messages::TestStep* testStep;
    };

    struct StringIdCompare
    {
        auto operator()(const std::string& lhs, const std::string& rhs) const -> bool
        {
            return std::stoi(lhs) < std::stoi(rhs);
        }
    };

    template<typename T>
    using Pointers = std::vector<const T*>;

    template<typename T>
    using ById = std::map<std::string, const T*, StringIdCompare>;

    template<typename T>
    using ManyById = std::map<std::string, Pointers<T>, StringIdCompare>;

    template<typename T>
    using Predicate = std::function<bool(const T&)>;

    // All views below are lazy and non-owning, except OwningView, and yield `const T&` on iteration.
    template<typename T>
    using ElementsView = TransformView<RefView<const Pointers<T>>, views::SelectPointee>;

    template<typename T>
    using FilteredElementsView = FilterView<ElementsView<T>, Predicate<T>>;

    template<typename T>
    using ValuesView = TransformView<TransformView<RefView<const ById<T>>, views::SelectSecond>, views::SelectPointee>;

    template<typename T>
    using FilteredValuesView = FilterView<ValuesView<T>, Predicate<T>>;

    template<typename T>
    using JoinedValuesView = TransformView<JoinView<TransformView<RefView<const ManyById<T>>, views::SelectSecond>>, views::SelectPointee>;

    template<typename T>
    using OwningView = TransformView<std::vector<const T*>, views::SelectPointee>;

    class Query
    {
    public:
        auto Update(const std::shared_ptr<const cucumber::messages::Envelope>& envelope) -> void;
        auto Update(const cucumber::messages::Envelope& envelope) -> void;

        [[nodiscard]] auto CountMostSevereTestStepResultStatus() const -> std::unordered_map<messages::TestStepResultStatus, std::size_t>;

        [[nodiscard]] auto CountTestCasesStarted() const -> std::size_t;

        [[nodiscard]] auto FindAllPickles() const -> ValuesView<messages::Pickle>;

        [[nodiscard]] auto FindAllPickleSteps() const -> ValuesView<messages::PickleStep>;

        [[nodiscard]] auto FindAllStepDefinitions() const -> ValuesView<messages::StepDefinition>;

        [[nodiscard]] auto FindAllTestCaseStarted() const -> FilteredValuesView<messages::TestCaseStarted>;

        [[nodiscard]] auto FindAllTestCaseFinished() const -> FilteredValuesView<messages::TestCaseFinished>;

        template<typename Transform, typename Cmp>
        [[nodiscard]] auto FindAllTestCaseStartedOrderBy(Transform&& findOrderBy, Cmp order) const -> OwningView<messages::TestCaseStarted>;

        template<typename Transform, typename Cmp>
        [[nodiscard]] auto FindAllTestCaseFinishedOrderBy(Transform&& findOrderBy, Cmp order) const -> OwningView<messages::TestCaseFinished>;

        [[nodiscard]] auto FindAllTestSteps() const -> ValuesView<messages::TestStep>;

        [[nodiscard]] auto FindAllTestCases() const -> ValuesView<messages::TestCase>;

        [[nodiscard]] auto FindAllTestStepStarted() const -> JoinedValuesView<messages::TestStepStarted>;

        [[nodiscard]] auto FindAllTestStepFinished() const -> JoinedValuesView<messages::TestStepFinished>;

        [[nodiscard]] auto FindAllTestRunHookStarted() const -> ValuesView<messages::TestRunHookStarted>;

        [[nodiscard]] auto FindAllTestRunHookFinished() const -> ValuesView<messages::TestRunHookFinished>;

        [[nodiscard]] auto FindAllUndefinedParameterTypes() const -> ElementsView<messages::UndefinedParameterType>;

        [[nodiscard]] auto FindAttachmentsBy(const messages::TestStepFinished& element) const -> FilteredElementsView<messages::Attachment>;
        [[nodiscard]] auto FindAttachmentsBy(const messages::TestRunHookFinished& element) const -> ElementsView<messages::Attachment>;

        [[nodiscard]] auto FindHookBy(const messages::TestStep& element) const -> const messages::Hook*;
        [[nodiscard]] auto FindHookBy(const messages::TestRunHookStarted& element) const -> const messages::Hook*;
        [[nodiscard]] auto FindHookBy(const messages::TestRunHookFinished& element) const -> const messages::Hook*;

        [[nodiscard]] auto FindMeta() const -> const messages::Meta*;

        [[nodiscard]] auto FindMostSevereTestStepResultBy(const messages::TestCaseStarted& element) const -> const messages::TestStepResult*;
        [[nodiscard]] auto FindMostSevereTestStepResultBy(const messages::TestCaseFinished& element) const -> const messages::TestStepResult*;

        [[nodiscard]] auto FindLocationOf(const messages::Pickle& pickle) const -> const messages::Location*;

        [[nodiscard]] auto FindPickleBy(const messages::TestCaseStarted& element) const -> const messages::Pickle*;
        [[nodiscard]] auto FindPickleBy(const messages::TestCaseFinished& element) const -> const messages::Pickle*;
        [[nodiscard]] auto FindPickleBy(const messages::TestStepStarted& element) const -> const messages::Pickle*;
        [[nodiscard]] auto FindPickleBy(const messages::TestStepFinished& element) const -> const messages::Pickle*;

        [[nodiscard]] auto FindPickleStepBy(const messages::TestStep& testStep) const -> const messages::PickleStep*;

        [[nodiscard]] auto FindStepBy(const messages::PickleStep& pickleStep) const -> const messages::Step*;

        [[nodiscard]] auto FindStepDefinitionsBy(const messages::TestStep& testStep) const -> OwningView<messages::StepDefinition>;

        [[nodiscard]] auto FindSuggestionsBy(const messages::PickleStep& element) const -> OwningView<messages::Suggestion>;
        [[nodiscard]] auto FindSuggestionsBy(const messages::Pickle& element) const -> OwningView<messages::Suggestion>;

        [[nodiscard]] auto FindUnambiguousStepDefinitionBy(const messages::TestStep& testStep) const -> const messages::StepDefinition*;

        [[nodiscard]] auto FindTestCaseBy(const messages::TestCaseStarted& element) const -> const messages::TestCase*;
        [[nodiscard]] auto FindTestCaseBy(const messages::TestCaseFinished& element) const -> const messages::TestCase*;
        [[nodiscard]] auto FindTestCaseBy(const messages::TestStepStarted& element) const -> const messages::TestCase*;
        [[nodiscard]] auto FindTestCaseBy(const messages::TestStepFinished& element) const -> const messages::TestCase*;

        [[nodiscard]] auto FindTestCaseDurationBy(const messages::TestCaseStarted& element) const -> std::optional<messages::Duration>;

        [[nodiscard]] auto FindTestCaseDurationBy(const messages::TestCaseFinished& element) const -> std::optional<messages::Duration>;

        [[nodiscard]] auto FindTestCaseStartedBy(const messages::TestCaseFinished& element) const -> const messages::TestCaseStarted*;
        [[nodiscard]] auto FindTestCaseStartedBy(const messages::TestStepStarted& element) const -> const messages::TestCaseStarted*;
        [[nodiscard]] auto FindTestCaseStartedBy(const messages::TestStepFinished& element) const -> const messages::TestCaseStarted*;

        [[nodiscard]] auto FindTestCaseFinishedBy(const messages::TestCaseStarted& testCaseStarted) const -> const messages::TestCaseFinished*;

        [[nodiscard]] auto FindTestRunHookStartedBy(const messages::TestRunHookFinished& testRunHookFinished) const -> const messages::TestRunHookStarted*;

        [[nodiscard]] auto FindTestRunHookFinishedBy(const messages::TestRunHookStarted& testRunHookStarted) const -> const messages::TestRunHookFinished*;

        [[nodiscard]] auto FindTestRunDuration() const -> std::optional<messages::Duration>;

        [[nodiscard]] auto FindTestRunFinished() const -> const messages::TestRunFinished*;

        [[nodiscard]] auto FindTestRunStarted() const -> const messages::TestRunStarted*;

        [[nodiscard]] auto FindTestStepBy(const messages::TestStepStarted& element) const -> const messages::TestStep*;
        [[nodiscard]] auto FindTestStepBy(const messages::TestStepFinished& element) const -> const messages::TestStep*;

        [[nodiscard]] auto FindTestStepsStartedBy(const messages::TestCaseStarted& testCaseStarted) const -> ElementsView<messages::TestStepStarted>;
        [[nodiscard]] auto FindTestStepsStartedBy(const messages::TestCaseFinished& testCaseFinished) const -> ElementsView<messages::TestStepStarted>;

        [[nodiscard]] auto FindTestStepsFinishedBy(const messages::TestCaseStarted& element) const -> ElementsView<messages::TestStepFinished>;
        [[nodiscard]] auto FindTestStepsFinishedBy(const messages::TestCaseFinished& element) const -> ElementsView<messages::TestStepFinished>;

        [[nodiscard]] auto FindTestStepFinishedAndTestStepBy(const messages::TestCaseStarted& testCaseStarted) const -> std::vector<TestStepFinishedAndTestStep>;

        [[nodiscard]] auto FindLineageBy(const messages::Pickle& element) const -> std::optional<LineageAndPickle>;
        [[nodiscard]] auto FindLineageBy(const messages::TestCaseStarted& element) const -> std::optional<LineageAndPickle>;
        [[nodiscard]] auto FindLineageBy(const messages::TestCaseFinished& element) const -> std::optional<LineageAndPickle>;

    private:
        [[nodiscard]] auto AllTestCaseStarted() const -> std::vector<const messages::TestCaseStarted*>;
        [[nodiscard]] auto AllTestCaseFinished() const -> std::vector<const messages::TestCaseFinished*>;

        auto UpdateGherkinDocument(const messages::GherkinDocument& gherkinDocument) -> void;
        auto UpdateFeature(const messages::Feature& feature, Lineage lineage) -> void;
        auto UpdateRule(const messages::Rule& rule, Lineage lineage) -> void;
        auto UpdateScenario(const messages::Scenario& scenario, const Lineage& lineage) -> void;
        auto UpdateSteps(const std::vector<messages::Step>& steps) -> void;
        auto UpdatePickle(const messages::Pickle& pickle) -> void;
        auto UpdateTestRunHookStarted(const messages::TestRunHookStarted& testRunHookStarted) -> void;
        auto UpdateTestRunHookFinished(const messages::TestRunHookFinished& testRunHookFinished) -> void;
        auto UpdateTestCase(const messages::TestCase& testCase) -> void;
        auto UpdateTestCaseStarted(const messages::TestCaseStarted& testCaseStarted) -> void;
        auto UpdateAttachment(const messages::Attachment& attachment) -> void;
        auto UpdateTestStepFinished(const messages::TestStepFinished& testStepFinished) -> void;
        auto UpdateTestCaseFinished(const messages::TestCaseFinished& testCaseFinished) -> void;

        const messages::Meta* meta{ nullptr };

        const messages::TestRunStarted* testRunStarted{ nullptr };
        const messages::TestRunFinished* testRunFinished{ nullptr };

        ById<messages::TestCaseStarted> testCaseStartedById;
        std::map<std::string, Lineage, StringIdCompare> lineageById;
        ById<messages::Step> stepById;
        ById<messages::Pickle> pickleById;
        ById<messages::PickleStep> pickleStepById;
        ById<messages::Hook> hookById;
        ById<messages::StepDefinition> stepDefinitionById;
        ById<messages::TestCase> testCaseById;
        ById<messages::TestStep> testStepById;
        ById<messages::TestCaseFinished> testCaseFinishedByTestCaseStartedId;
        ById<messages::TestRunHookStarted> testRunHookStartedById;
        ById<messages::TestRunHookFinished> testRunHookFinishedByTestRunHookStartedId;
        ManyById<messages::TestStepStarted> testStepStartedByTestCaseStartedId;
        ManyById<messages::TestStepFinished> testStepFinishedByTestCaseStartedId;
        ManyById<messages::Attachment> attachmentsByTestCaseStartedId;
        ManyById<messages::Attachment> attachmentsByTestRunHookStartedId;
        ById<messages::Suggestion> suggestionsByPickleStepId;
        Pointers<messages::UndefinedParameterType> undefinedParameterTypes;
    };

    static inline const messages::Pickle* (query::Query::* const findPickleByTestCaseFinished)(const messages::TestCaseFinished&) const = &query::Query::FindPickleBy;

    namespace detail
    {
        template<typename TElement, typename Transform, typename Cmp>
        [[nodiscard]] auto FindAllOrderBy(const Query& query, std::vector<const TElement*> allElements, Transform findOrderBy, Cmp order) -> OwningView<TElement>
        {
            using TransformResult = decltype(std::invoke(findOrderBy, query, std::declval<const TElement&>()));

            std::vector<std::pair<const TElement*, TransformResult>> transformed;
            transformed.reserve(allElements.size());

            for (const auto* element : allElements)
            {
                transformed.emplace_back(element, std::invoke(findOrderBy, query, *element));
            }

            std::sort(transformed.begin(), transformed.end(),
                [&](const auto& lhs, const auto& rhs)
                {
                    if (lhs.second == nullptr && rhs.second == nullptr)
                    {
                        return false;
                    }
                    if (lhs.second == nullptr)
                    {
                        return true;
                    }
                    if (rhs.second == nullptr)
                    {
                        return false;
                    }

                    return std::invoke(order, *lhs.second, *rhs.second) < 0;
                });

            std::vector<const TElement*> result;
            result.reserve(transformed.size());
            for (const auto& pair : transformed)
            {
                result.push_back(pair.first);
            }

            return OwningView<TElement>{ std::move(result), views::SelectPointee{} };
        }
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] auto Query::FindAllTestCaseStartedOrderBy(Transform&& findOrderBy, Cmp order) const -> OwningView<messages::TestCaseStarted>
    {
        return detail::FindAllOrderBy<messages::TestCaseStarted>(*this, AllTestCaseStarted(), std::forward<Transform>(findOrderBy), std::move(order));
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] auto Query::FindAllTestCaseFinishedOrderBy(Transform&& findOrderBy, Cmp order) const -> OwningView<messages::TestCaseFinished>
    {
        return detail::FindAllOrderBy<messages::TestCaseFinished>(*this, AllTestCaseFinished(), std::forward<Transform>(findOrderBy), std::move(order));
    }
}

#endif
