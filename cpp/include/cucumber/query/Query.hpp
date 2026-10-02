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
        bool operator()(const std::string& lhs, const std::string& rhs) const
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
        void Update(const std::shared_ptr<const cucumber::messages::Envelope>& envelope);
        void Update(const cucumber::messages::Envelope& envelope);

        [[nodiscard]] std::unordered_map<messages::TestStepResultStatus, std::size_t> CountMostSevereTestStepResultStatus() const;

        [[nodiscard]] std::size_t CountTestCasesStarted() const;

        [[nodiscard]] ValuesView<messages::Pickle> FindAllPickles() const;

        [[nodiscard]] ValuesView<messages::PickleStep> FindAllPickleSteps() const;

        [[nodiscard]] ValuesView<messages::StepDefinition> FindAllStepDefinitions() const;

        [[nodiscard]] FilteredValuesView<messages::TestCaseStarted> FindAllTestCaseStarted() const;

        [[nodiscard]] FilteredValuesView<messages::TestCaseFinished> FindAllTestCaseFinished() const;

        template<typename Transform, typename Cmp>
        [[nodiscard]] OwningView<messages::TestCaseStarted> FindAllTestCaseStartedOrderBy(Transform&& findOrderBy, Cmp order) const;

        template<typename Transform, typename Cmp>
        [[nodiscard]] OwningView<messages::TestCaseFinished> FindAllTestCaseFinishedOrderBy(Transform&& findOrderBy, Cmp order) const;

        [[nodiscard]] ValuesView<messages::TestStep> FindAllTestSteps() const;

        [[nodiscard]] ValuesView<messages::TestCase> FindAllTestCases() const;

        [[nodiscard]] JoinedValuesView<messages::TestStepStarted> FindAllTestStepStarted() const;

        [[nodiscard]] JoinedValuesView<messages::TestStepFinished> FindAllTestStepFinished() const;

        [[nodiscard]] ValuesView<messages::TestRunHookStarted> FindAllTestRunHookStarted() const;

        [[nodiscard]] ValuesView<messages::TestRunHookFinished> FindAllTestRunHookFinished() const;

        [[nodiscard]] ElementsView<messages::UndefinedParameterType> FindAllUndefinedParameterTypes() const;

        [[nodiscard]] FilteredElementsView<messages::Attachment> FindAttachmentsBy(const messages::TestStepFinished& element) const;
        [[nodiscard]] ElementsView<messages::Attachment> FindAttachmentsBy(const messages::TestRunHookFinished& element) const;

        [[nodiscard]] const messages::Hook* FindHookBy(const messages::TestStep& element) const;
        [[nodiscard]] const messages::Hook* FindHookBy(const messages::TestRunHookStarted& element) const;
        [[nodiscard]] const messages::Hook* FindHookBy(const messages::TestRunHookFinished& element) const;

        [[nodiscard]] const messages::Meta* FindMeta() const;

        [[nodiscard]] const messages::TestStepResult* FindMostSevereTestStepResultBy(const messages::TestCaseStarted& element) const;
        [[nodiscard]] const messages::TestStepResult* FindMostSevereTestStepResultBy(const messages::TestCaseFinished& element) const;

        [[nodiscard]] const messages::Location* FindLocationOf(const messages::Pickle& pickle) const;

        [[nodiscard]] const messages::Pickle* FindPickleBy(const messages::TestCaseStarted& element) const;
        [[nodiscard]] const messages::Pickle* FindPickleBy(const messages::TestCaseFinished& element) const;
        [[nodiscard]] const messages::Pickle* FindPickleBy(const messages::TestStepStarted& element) const;
        [[nodiscard]] const messages::Pickle* FindPickleBy(const messages::TestStepFinished& element) const;

        [[nodiscard]] const messages::PickleStep* FindPickleStepBy(const messages::TestStep& testStep) const;

        [[nodiscard]] const messages::Step* FindStepBy(const messages::PickleStep& pickleStep) const;

        [[nodiscard]] OwningView<messages::StepDefinition> FindStepDefinitionsBy(const messages::TestStep& testStep) const;

        [[nodiscard]] OwningView<messages::Suggestion> FindSuggestionsBy(const messages::PickleStep& element) const;
        [[nodiscard]] OwningView<messages::Suggestion> FindSuggestionsBy(const messages::Pickle& element) const;

        [[nodiscard]] const messages::StepDefinition* FindUnambiguousStepDefinitionBy(const messages::TestStep& testStep) const;

        [[nodiscard]] const messages::TestCase* FindTestCaseBy(const messages::TestCaseStarted& element) const;
        [[nodiscard]] const messages::TestCase* FindTestCaseBy(const messages::TestCaseFinished& element) const;
        [[nodiscard]] const messages::TestCase* FindTestCaseBy(const messages::TestStepStarted& element) const;
        [[nodiscard]] const messages::TestCase* FindTestCaseBy(const messages::TestStepFinished& element) const;

        [[nodiscard]] std::optional<messages::Duration> FindTestCaseDurationBy(const messages::TestCaseStarted& element) const;

        [[nodiscard]] std::optional<messages::Duration> FindTestCaseDurationBy(const messages::TestCaseFinished& element) const;

        [[nodiscard]] const messages::TestCaseStarted* FindTestCaseStartedBy(const messages::TestCaseFinished& element) const;
        [[nodiscard]] const messages::TestCaseStarted* FindTestCaseStartedBy(const messages::TestStepStarted& element) const;
        [[nodiscard]] const messages::TestCaseStarted* FindTestCaseStartedBy(const messages::TestStepFinished& element) const;

        [[nodiscard]] const messages::TestCaseFinished* FindTestCaseFinishedBy(const messages::TestCaseStarted& testCaseStarted) const;

        [[nodiscard]] const messages::TestRunHookStarted* FindTestRunHookStartedBy(const messages::TestRunHookFinished& testRunHookFinished) const;

        [[nodiscard]] const messages::TestRunHookFinished* FindTestRunHookFinishedBy(const messages::TestRunHookStarted& testRunHookStarted) const;

        [[nodiscard]] std::optional<messages::Duration> FindTestRunDuration() const;

        [[nodiscard]] const messages::TestRunFinished* FindTestRunFinished() const;

        [[nodiscard]] const messages::TestRunStarted* FindTestRunStarted() const;

        [[nodiscard]] const messages::TestStep* FindTestStepBy(const messages::TestStepStarted& element) const;
        [[nodiscard]] const messages::TestStep* FindTestStepBy(const messages::TestStepFinished& element) const;

        [[nodiscard]] ElementsView<messages::TestStepStarted> FindTestStepsStartedBy(const messages::TestCaseStarted& testCaseStarted) const;
        [[nodiscard]] ElementsView<messages::TestStepStarted> FindTestStepsStartedBy(const messages::TestCaseFinished& testCaseFinished) const;

        [[nodiscard]] ElementsView<messages::TestStepFinished> FindTestStepsFinishedBy(const messages::TestCaseStarted& element) const;
        [[nodiscard]] ElementsView<messages::TestStepFinished> FindTestStepsFinishedBy(const messages::TestCaseFinished& element) const;

        [[nodiscard]] std::vector<TestStepFinishedAndTestStep> FindTestStepFinishedAndTestStepBy(const messages::TestCaseStarted& testCaseStarted) const;

        [[nodiscard]] std::optional<LineageAndPickle> FindLineageBy(const messages::Pickle& element) const;
        [[nodiscard]] std::optional<LineageAndPickle> FindLineageBy(const messages::TestCaseStarted& element) const;
        [[nodiscard]] std::optional<LineageAndPickle> FindLineageBy(const messages::TestCaseFinished& element) const;

    private:
        [[nodiscard]] std::vector<const messages::TestCaseStarted*> AllTestCaseStarted() const;
        [[nodiscard]] std::vector<const messages::TestCaseFinished*> AllTestCaseFinished() const;

        void UpdateGherkinDocument(const messages::GherkinDocument& gherkinDocument);
        void UpdateFeature(const messages::Feature& feature, Lineage lineage);
        void UpdateRule(const messages::Rule& rule, Lineage lineage);
        void UpdateScenario(const messages::Scenario& scenario, const Lineage& lineage);
        void UpdateSteps(const std::vector<messages::Step>& steps);
        void UpdatePickle(const messages::Pickle& pickle);
        void UpdateTestRunHookStarted(const messages::TestRunHookStarted& testRunHookStarted);
        void UpdateTestRunHookFinished(const messages::TestRunHookFinished& testRunHookFinished);
        void UpdateTestCase(const messages::TestCase& testCase);
        void UpdateTestCaseStarted(const messages::TestCaseStarted& testCaseStarted);
        void UpdateAttachment(const messages::Attachment& attachment);
        void UpdateTestStepFinished(const messages::TestStepFinished& testStepFinished);
        void UpdateTestCaseFinished(const messages::TestCaseFinished& testCaseFinished);

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
        [[nodiscard]] OwningView<TElement> FindAllOrderBy(const Query& query, std::vector<const TElement*> allElements, Transform findOrderBy, Cmp order)
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
    [[nodiscard]] OwningView<messages::TestCaseStarted> Query::FindAllTestCaseStartedOrderBy(Transform&& findOrderBy, Cmp order) const
    {
        return detail::FindAllOrderBy<messages::TestCaseStarted>(*this, AllTestCaseStarted(), std::forward<Transform>(findOrderBy), std::move(order));
    }

    template<typename Transform, typename Cmp>
    [[nodiscard]] OwningView<messages::TestCaseFinished> Query::FindAllTestCaseFinishedOrderBy(Transform&& findOrderBy, Cmp order) const
    {
        return detail::FindAllOrderBy<messages::TestCaseFinished>(*this, AllTestCaseFinished(), std::forward<Transform>(findOrderBy), std::move(order));
    }
}

#endif
