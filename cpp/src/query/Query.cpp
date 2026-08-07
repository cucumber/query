#include "cucumber/query/Query.hpp"
#include "cucumber/messages/All.hpp"
#include "cucumber/messages/DurationUtil.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/Lineage.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        template<class T, typename Proj>
        auto SortBy(std::vector<T>& container, const Proj& projection) -> void
        {
            std::sort(container.begin(), container.end(),
                [&projection](const auto& lhs, const auto& rhs)
                {
                    return std::stoi(std::invoke(projection, lhs)) < std::stoi(std::invoke(projection, rhs));
                });
        }

        auto SortBySeverity(std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>>& container) -> void
        {
            std::sort(container.begin(), container.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    using underlying_type = std::underlying_type_t<messages::TestStepResultStatus>;
                    return static_cast<underlying_type>(lhs.first->testStepResult->status) > static_cast<underlying_type>(rhs.first->testStepResult->status);
                });
        }

        template<typename T>
        auto MapValuesToVector(const std::unordered_map<std::string, T>& container)
        {
            std::vector<T> result;
            result.reserve(container.size());

            for (const auto& [key, value] : container)
            {
                result.push_back(value);
            }

            return result;
        }

        template<typename T, typename Proj>
        auto MapValuesToVectorSortBy(const std::unordered_map<std::string, T>& container, const Proj& projection)
        {
            auto result = MapValuesToVector(container);

            SortBy(result, projection);

            return result;
        }

        template<class T>
        auto MapValuesToVector(const std::unordered_map<std::string, std::vector<T>>& container)
        {
            std::vector<T> result;
            result.reserve(container.size());

            for (const auto& [key, value] : container)
            {
                result.insert(result.end(), value.begin(), value.end());
            }

            return result;
        }

        template<typename T, typename Proj>
        auto MapValuesToVectorSortBy(const std::unordered_map<std::string, std::vector<T>>& container, const Proj& projection)
        {
            auto result = MapValuesToVector(container);

            SortBy(result, projection);

            return result;
        }

        template<class... Ts>
        struct overloaded : Ts...
        {
            using Ts::operator()...;
        };
        // explicit deduction guide (not needed as of C++20)
        template<class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;
    }

    auto Query::Update(const cucumber::messages::Envelope& envelope) -> void
    {
        if (envelope.meta.has_value())
        {
            meta = envelope.meta.value();
        }
        if (envelope.gherkinDocument.has_value())
        {
            UpdateGherkinDocument(envelope.gherkinDocument.value());
        }
        if (envelope.pickle.has_value())
        {
            UpdatePickle(envelope.pickle.value());
        }
        if (envelope.hook.has_value())
        {
            hookById[envelope.hook.value()->id] = envelope.hook.value();
        }
        if (envelope.stepDefinition.has_value())
        {
            stepDefinitionById[envelope.stepDefinition.value()->id] = envelope.stepDefinition.value();
        }
        if (envelope.testRunStarted.has_value())
        {
            testRunStarted = envelope.testRunStarted.value();
        }
        if (envelope.testRunHookStarted.has_value())
        {
            UpdateTestRunHookStarted(envelope.testRunHookStarted.value());
        }
        if (envelope.testRunHookFinished.has_value())
        {
            UpdateTestRunHookFinished(envelope.testRunHookFinished.value());
        }
        if (envelope.testCase.has_value())
        {
            UpdateTestCase(envelope.testCase.value());
        }
        if (envelope.testCaseStarted.has_value())
        {
            UpdateTestCaseStarted(envelope.testCaseStarted.value());
        }
        if (envelope.testStepStarted.has_value())
        {
            testStepStartedByTestCaseStartedId[envelope.testStepStarted.value()->testCaseStartedId].push_back(envelope.testStepStarted.value());
        }
        if (envelope.attachment.has_value())
        {
            UpdateAttachment(envelope.attachment.value());
        }
        if (envelope.testStepFinished.has_value())
        {
            UpdateTestStepFinished(envelope.testStepFinished.value());
        }
        if (envelope.testCaseFinished.has_value())
        {
            UpdateTestCaseFinished(envelope.testCaseFinished.value());
        }
        if (envelope.testRunFinished.has_value())
        {
            testRunFinished = envelope.testRunFinished.value();
        }
        if (envelope.suggestion.has_value())
        {
            suggestionsByPickleStepId[envelope.suggestion.value()->pickleStepId] = envelope.suggestion.value();
        }
        if (envelope.undefinedParameterType.has_value())
        {
            undefinedParameterTypes.push_back(envelope.undefinedParameterType.value());
        }
    }

    auto Query::CountMostSevereTestStepResultStatus() const -> std::unordered_map<messages::TestStepResultStatus, std::size_t>
    {
        std::unordered_map<messages::TestStepResultStatus, std::size_t> result{
            { messages::TestStepResultStatus::AMBIGUOUS, 0 },
            { messages::TestStepResultStatus::FAILED, 0 },
            { messages::TestStepResultStatus::PASSED, 0 },
            { messages::TestStepResultStatus::PENDING, 0 },
            { messages::TestStepResultStatus::SKIPPED, 0 },
            { messages::TestStepResultStatus::UNDEFINED, 0 },
            { messages::TestStepResultStatus::UNKNOWN, 0 },
        };

        for (const auto& testCaseStarted : FindAllTestCaseStarted())
        {
            auto testStepFinishedAndTestStep = FindTestStepFinishedAndTestStepBy(testCaseStarted);
            if (!testStepFinishedAndTestStep.empty())
            {
                SortBySeverity(testStepFinishedAndTestStep);

                ++result[testStepFinishedAndTestStep.front().first->testStepResult->status];
            }
        }

        return result;
    }

    auto Query::CountTestCasesStarted() const -> std::size_t
    {
        return FindAllTestCaseStarted().size();
    }

    auto Query::FindAllPickles() const -> std::vector<std::shared_ptr<const messages::Pickle>>
    {
        return MapValuesToVectorSortBy(pickleById, &messages::Pickle::id);
    }

    auto Query::FindAllPickleSteps() const -> std::vector<std::shared_ptr<const messages::PickleStep>>
    {
        return MapValuesToVectorSortBy(pickleStepById, &messages::PickleStep::id);
    }

    auto Query::FindAllStepDefinitions() const -> std::vector<std::shared_ptr<const messages::StepDefinition>>
    {
        return MapValuesToVectorSortBy(stepDefinitionById, &messages::StepDefinition::id);
    }

    auto Query::FindAllTestCaseStarted() const -> std::vector<std::shared_ptr<const messages::TestCaseStarted>>
    {
        std::vector<std::shared_ptr<const messages::TestCaseStarted>> result;

        for (const auto& [testCaseStartedId, testCaseStarted] : testCaseStartedById)
        {
            auto iter = testCaseFinishedByTestCaseStartedId.find(testCaseStarted->id);

            if (iter == testCaseFinishedByTestCaseStartedId.end() || !iter->second->willBeRetried)
            {
                result.push_back(testCaseStarted);
            }
        }

        SortBy(result, &messages::TestCaseStarted::id);

        return result;
    }

    auto Query::FindAllTestCaseFinished() const -> std::vector<std::shared_ptr<const messages::TestCaseFinished>>
    {
        std::vector<std::shared_ptr<const messages::TestCaseFinished>> result;

        for (const auto& [testCaseStartedId, testCaseFinished] : testCaseFinishedByTestCaseStartedId)
        {
            if (!testCaseFinished->willBeRetried)
            {
                result.push_back(testCaseFinished);
            }
        }

        SortBy(result, &messages::TestCaseFinished::testCaseStartedId);

        return result;
    }

    auto Query::FindAllTestSteps() const -> std::vector<std::shared_ptr<const messages::TestStep>>
    {
        return MapValuesToVectorSortBy(testStepById, &messages::TestStep::id);
    }

    auto Query::FindAllTestCases() const -> std::vector<std::shared_ptr<const messages::TestCase>>
    {
        return MapValuesToVectorSortBy(testCaseById, &messages::TestCase::id);
    }

    auto Query::FindAllTestStepStarted() const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>
    {
        return MapValuesToVectorSortBy(testStepStartedByTestCaseStartedId, &messages::TestStepStarted::testCaseStartedId);
    }

    auto Query::FindAllTestStepFinished() const -> std::vector<std::shared_ptr<const messages::TestStepFinished>>
    {
        return MapValuesToVectorSortBy(testStepFinishedByTestCaseStartedId, &messages::TestStepFinished::testCaseStartedId);
    }

    auto Query::FindAllTestRunHookStarted() const -> std::vector<std::shared_ptr<const messages::TestRunHookStarted>>
    {
        return MapValuesToVectorSortBy(testRunHookStartedById, &messages::TestRunHookStarted::id);
    }

    auto Query::FindAllTestRunHookFinished() const -> std::vector<std::shared_ptr<const messages::TestRunHookFinished>>
    {
        return MapValuesToVectorSortBy(testRunHookFinishedByTestRunHookStartedId, &messages::TestRunHookFinished::testRunHookStartedId);
    }

    auto Query::FindAllUndefinedParameterTypes() const -> std::vector<std::shared_ptr<const messages::UndefinedParameterType>>
    {
        return undefinedParameterTypes;
    }

    auto Query::FindAttachmentsBy(const std::shared_ptr<const messages::TestStepFinished>& element) const -> std::vector<std::shared_ptr<const messages::Attachment>>
    {
        std::vector<std::shared_ptr<const messages::Attachment>> result;
        if (attachmentsByTestCaseStartedId.find(element->testCaseStartedId) != attachmentsByTestCaseStartedId.end())
        {
            for (const auto& attachment : attachmentsByTestCaseStartedId.at(element->testCaseStartedId))
            {
                if (attachment->testStepId == element->testStepId)
                {
                    result.push_back(attachment);
                }
            }
        }
        return result;
    }

    auto Query::FindAttachmentsBy(const std::shared_ptr<const messages::TestRunHookFinished>& element) const -> std::vector<std::shared_ptr<const messages::Attachment>>
    {
        if (attachmentsByTestRunHookStartedId.find(element->testRunHookStartedId) != attachmentsByTestRunHookStartedId.end())
        {
            return attachmentsByTestRunHookStartedId.at(element->testRunHookStartedId);
        }
        return {};
    }

    auto Query::FindHookBy(const std::shared_ptr<const messages::TestStep>& element) const -> std::optional<std::shared_ptr<const messages::Hook>>
    {
        if (element->hookId.has_value())
        {
            return hookById.at(element->hookId.value());
        }
        return std::nullopt;
    }

    auto Query::FindHookBy(const std::shared_ptr<const messages::TestRunHookStarted>& element) const -> std::optional<std::shared_ptr<const messages::Hook>>
    {
        if (hookById.find(element->hookId) != hookById.end())
        {
            return hookById.at(element->hookId);
        }
        return std::nullopt;
    }

    auto Query::FindHookBy(const std::shared_ptr<const messages::TestRunHookFinished>& element) const -> std::optional<std::shared_ptr<const messages::Hook>>
    {
        const auto testRunHookStarted = FindTestRunHookStartedBy(element);
        if (!testRunHookStarted.has_value())
        {
            throw std::out_of_range{ "Expected to find TestRunHookStarted from TestRunHookFinished" };
        }
        return FindHookBy(testRunHookStarted.value());
    }

    auto Query::FindMeta() const -> std::optional<std::shared_ptr<const messages::Meta>>
    {
        return meta;
    }

    auto Query::FindMostSevereTestStepResultBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<std::shared_ptr<const messages::TestStepResult>>
    {
        auto testStepFinishedAndTestStep = FindTestStepFinishedAndTestStepBy(element);
        if (!testStepFinishedAndTestStep.empty())
        {
            SortBySeverity(testStepFinishedAndTestStep);

            return testStepFinishedAndTestStep.front().first->testStepResult;
        }
        return std::nullopt;
    }

    auto Query::FindMostSevereTestStepResultBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestStepResult>>
    {
        const auto testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted.has_value())
        {
            return FindMostSevereTestStepResultBy(testCaseStarted.value());
        }
        return std::nullopt;
    }

    auto Query::FindLocationOf(const std::shared_ptr<const messages::Pickle>& pickle) const -> std::optional<std::shared_ptr<const messages::Location>>
    {
        if (pickle->location.has_value())
        {
            return pickle->location.value();
        }

        const auto lineageAndPickle = FindLineageBy(pickle);

        if (lineageAndPickle.has_value())
        {
            const auto& lineage = lineageAndPickle.value().lineage;

            if (lineage->example)
            {
                return lineage->example->location;
            }

            if (lineage->scenario)
            {
                return lineage->scenario->location;
            }
        }

        return std::nullopt;
    }

    auto Query::FindPickleBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<std::shared_ptr<const messages::Pickle>>
    {
        const auto& testCase = FindTestCaseBy(element);
        if (testCase.has_value())
        {
            return pickleById.at(testCase.value()->pickleId);
        }
        return std::nullopt;
    }

    auto Query::FindPickleBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::Pickle>>
    {
        const auto& testCase = FindTestCaseBy(element);
        if (testCase.has_value())
        {
            return pickleById.at(testCase.value()->pickleId);
        }
        return std::nullopt;
    }

    auto Query::FindPickleBy(const std::shared_ptr<const messages::TestStepStarted>& element) const -> std::optional<std::shared_ptr<const messages::Pickle>>
    {
        const auto& testCase = FindTestCaseBy(element);
        if (testCase.has_value())
        {
            return pickleById.at(testCase.value()->pickleId);
        }
        return std::nullopt;
    }

    auto Query::FindPickleBy(const std::shared_ptr<const messages::TestStepFinished>& element) const -> std::optional<std::shared_ptr<const messages::Pickle>>
    {
        const auto& testCase = FindTestCaseBy(element);
        if (testCase.has_value())
        {
            return pickleById.at(testCase.value()->pickleId);
        }
        return std::nullopt;
    }

    auto Query::FindPickleStepBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::optional<std::shared_ptr<const messages::PickleStep>>
    {
        if (testStep->pickleStepId.has_value())
        {
            return pickleStepById.at(testStep->pickleStepId.value());
        }
        return std::nullopt;
    }

    auto Query::FindStepBy(const std::shared_ptr<const messages::PickleStep>& pickleStep) const -> std::optional<std::shared_ptr<const messages::Step>>
    {
        const auto stepId = pickleStep->astNodeIds.front();

        if (stepById.find(stepId) != stepById.end())
        {
            return stepById.at(pickleStep->astNodeIds.front());
        }
        return std::nullopt;
    }

    auto Query::FindStepDefinitionsBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::vector<std::shared_ptr<const messages::StepDefinition>>
    {
        std::vector<std::shared_ptr<const messages::StepDefinition>> result;

        if (testStep->stepDefinitionIds.has_value())
        {
            for (const auto& stepDefinitionId : testStep->stepDefinitionIds.value())
            {
                if (stepDefinitionById.find(stepDefinitionId) != stepDefinitionById.end())
                {
                    result.push_back(stepDefinitionById.at(stepDefinitionId));
                }
            }
        }

        return result;
    }

    auto Query::FindSuggestionsBy(const std::shared_ptr<const messages::PickleStep>& element) const -> std::vector<std::shared_ptr<const messages::Suggestion>>
    {
        if (suggestionsByPickleStepId.find(element->id) != suggestionsByPickleStepId.end())
        {
            return { suggestionsByPickleStepId.at(element->id) };
        }

        return {};
    }

    auto Query::FindSuggestionsBy(const std::shared_ptr<const messages::Pickle>& element) const -> std::vector<std::shared_ptr<const messages::Suggestion>>
    {
        std::vector<std::shared_ptr<const messages::Suggestion>> result;
        for (const auto& pickleStep : element->steps)
        {
            const auto suggestions = FindSuggestionsBy(pickleStep);
            result.insert(result.end(), suggestions.begin(), suggestions.end());
        }
        return result;
    }

    auto Query::FindUnambiguousStepDefinitionBy(const std::shared_ptr<const messages::TestStep>& testStep) const -> std::optional<std::shared_ptr<const messages::StepDefinition>>
    {
        if (testStep->stepDefinitionIds.has_value() && testStep->stepDefinitionIds.value().size() == 1)
        {
            const auto stepDefinitionId = testStep->stepDefinitionIds.value().front();
            if (stepDefinitionById.find(stepDefinitionId) != stepDefinitionById.end())
            {
                return stepDefinitionById.at(stepDefinitionId);
            }
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<std::shared_ptr<const messages::TestCase>>
    {
        if (testCaseById.find(element->testCaseId) != testCaseById.end())
        {
            return testCaseById.at(element->testCaseId);
        }

        return std::nullopt;
    }

    auto Query::FindTestCaseBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestCase>>
    {
        const auto& testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted.has_value())
        {
            return FindTestCaseBy(testCaseStarted.value());
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseBy(const std::shared_ptr<const messages::TestStepStarted>& element) const -> std::optional<std::shared_ptr<const messages::TestCase>>
    {
        const auto& testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted.has_value())
        {
            return FindTestCaseBy(testCaseStarted.value());
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseBy(const std::shared_ptr<const messages::TestStepFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestCase>>
    {
        const auto& testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted.has_value())
        {
            return FindTestCaseBy(testCaseStarted.value());
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<std::shared_ptr<const messages::Duration>>
    {
        const auto& testCaseFinished = FindTestCaseFinishedBy(element);
        if (testCaseFinished.has_value())
        {
            return std::make_shared<messages::Duration>(*testCaseFinished.value()->timestamp - *element->timestamp);
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::Duration>>
    {
        const auto testCaseStarted = FindTestCaseStartedBy(element);

        if (testCaseStarted.has_value())
        {
            return FindTestCaseDurationBy(testCaseStarted.value());
        }

        return std::nullopt;
    }

    auto Query::FindTestCaseStartedBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
    {
        const auto iter = testCaseStartedById.find(element->testCaseStartedId);
        if (iter != testCaseStartedById.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseStartedBy(const std::shared_ptr<const messages::TestStepStarted>& element) const -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
    {
        const auto iter = testCaseStartedById.find(element->testCaseStartedId);
        if (iter != testCaseStartedById.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseStartedBy(const std::shared_ptr<const messages::TestStepFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
    {
        const auto iter = testCaseStartedById.find(element->testCaseStartedId);
        if (iter != testCaseStartedById.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseFinishedBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const -> std::optional<std::shared_ptr<const messages::TestCaseFinished>>
    {
        if (testCaseFinishedByTestCaseStartedId.find(testCaseStarted->id) != testCaseFinishedByTestCaseStartedId.end())
        {
            return testCaseFinishedByTestCaseStartedId.at(testCaseStarted->id);
        }
        return std::nullopt;
    }

    auto Query::FindTestRunHookStartedBy(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) const -> std::optional<std::shared_ptr<const messages::TestRunHookStarted>>
    {
        const auto iter = testRunHookStartedById.find(testRunHookFinished->testRunHookStartedId);
        if (iter != testRunHookStartedById.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    auto Query::FindTestRunHookFinishedBy(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted) const -> std::optional<std::shared_ptr<const messages::TestRunHookFinished>>
    {
        if (testRunHookFinishedByTestRunHookStartedId.find(testRunHookStarted->id) != testRunHookFinishedByTestRunHookStartedId.end())
        {
            return testRunHookFinishedByTestRunHookStartedId.at(testRunHookStarted->id);
        }
        return std::nullopt;
    }

    auto Query::FindTestRunDuration() const -> std::optional<std::shared_ptr<const messages::Duration>>
    {
        if (testRunStarted.has_value() && testRunFinished.has_value())
        {
            return std::make_shared<messages::Duration>(*testRunFinished.value()->timestamp - *testRunStarted.value()->timestamp);
        }

        return std::nullopt;
    }

    auto Query::FindTestRunFinished() const -> std::optional<std::shared_ptr<const messages::TestRunFinished>>
    {
        return testRunFinished;
    }

    auto Query::FindTestRunStarted() const -> std::optional<std::shared_ptr<const messages::TestRunStarted>>
    {
        return testRunStarted;
    }

    auto Query::FindTestStepBy(const std::shared_ptr<const messages::TestStepStarted>& element) const -> std::optional<std::shared_ptr<const messages::TestStep>>
    {
        if (testStepById.find(element->testStepId) != testStepById.end())
        {
            return testStepById.at(element->testStepId);
        }
        return std::nullopt;
    }

    auto Query::FindTestStepBy(const std::shared_ptr<const messages::TestStepFinished>& element) const -> std::optional<std::shared_ptr<const messages::TestStep>>
    {
        if (testStepById.find(element->testStepId) != testStepById.end())
        {
            return testStepById.at(element->testStepId);
        }
        return std::nullopt;
    }

    auto Query::FindTestStepsStartedBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>
    {
        if (testStepStartedByTestCaseStartedId.find(testCaseStarted->id) != testStepStartedByTestCaseStartedId.end())
        {
            return testStepStartedByTestCaseStartedId.at(testCaseStarted->id);
        }
        return {};
    }

    auto Query::FindTestStepsStartedBy(const std::shared_ptr<const messages::TestCaseFinished>& testCaseFinished) const -> std::vector<std::shared_ptr<const messages::TestStepStarted>>
    {
        if (testStepStartedByTestCaseStartedId.find(testCaseFinished->testCaseStartedId) != testStepStartedByTestCaseStartedId.end())
        {
            return testStepStartedByTestCaseStartedId.at(testCaseFinished->testCaseStartedId);
        }
        return {};
    }

    auto Query::FindTestStepsFinishedBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::vector<std::shared_ptr<const messages::TestStepFinished>>
    {
        if (testStepFinishedByTestCaseStartedId.find(element->id) != testStepFinishedByTestCaseStartedId.end())
        {
            return testStepFinishedByTestCaseStartedId.at(element->id);
        }

        return {};
    }

    auto Query::FindTestStepsFinishedBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::vector<std::shared_ptr<const messages::TestStepFinished>>
    {
        const auto& testCaseStarted = FindTestCaseStartedBy(element);

        if (testCaseStarted.has_value())
        {
            return FindTestStepsFinishedBy(testCaseStarted.value());
        }

        return {};
    }

    auto Query::FindTestStepFinishedAndTestStepBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const
        -> std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>>
    {
        std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>> result;
        const auto testStepsFinishedIter = testStepFinishedByTestCaseStartedId.find(testCaseStarted->id);

        if (testStepsFinishedIter != testStepFinishedByTestCaseStartedId.end())
        {
            for (const auto& testStepFinished : testStepsFinishedIter->second)
            {
                const auto& testStep = FindTestStepBy(testStepFinished);
                if (!testStep)
                {
                    throw std::out_of_range{ "Expected to find TestStep by TestStepFinished" };
                }
                result.emplace_back(testStepFinished, *testStep);
            }
        }
        return result;
    }

    auto Query::FindLineageBy(const std::shared_ptr<const messages::Pickle>& element) const -> std::optional<LineageAndPickle>
    {
        if (lineageById.find(element->astNodeIds.back()) != lineageById.end())
        {
            return LineageAndPickle{ lineageById.at(element->astNodeIds.back()), element };
        }

        return std::nullopt;
    }

    auto Query::FindLineageBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const -> std::optional<LineageAndPickle>
    {
        const auto& pickle = FindPickleBy(element);

        if (pickle.has_value())
        {
            return FindLineageBy(pickle.value());
        }

        return std::nullopt;
    }

    auto Query::FindLineageBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const -> std::optional<LineageAndPickle>
    {
        const auto& pickle = FindPickleBy(element);

        if (pickle.has_value())
        {
            return FindLineageBy(pickle.value());
        }

        return std::nullopt;
    }

    auto Query::UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument) -> void
    {
        if (gherkinDocument->feature.has_value())
        {
            UpdateFeature(gherkinDocument->feature.value(), std::make_shared<Lineage>(Lineage{ gherkinDocument }));
        }
    }

    auto Query::UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, const std::shared_ptr<Lineage>& lineage) -> void
    {
        for (const auto& featureChild : feature->children)
        {
            if (featureChild->background.has_value())
            {
                lineage->background = featureChild->background.value();
                UpdateSteps(featureChild->background.value()->steps);
            }

            if (featureChild->scenario.has_value())
            {
                UpdateScenario(featureChild->scenario.value(), std::make_shared<Lineage>(*lineage + Lineage{ {}, feature }));
            }

            if (featureChild->rule.has_value())
            {
                UpdateRule(featureChild->rule.value(), std::make_shared<Lineage>(*lineage + Lineage{ {}, feature }));
            }
        }
    }

    auto Query::UpdateRule(const std::shared_ptr<const messages::Rule>& rule, const std::shared_ptr<Lineage>& lineage) -> void
    {
        for (const auto& ruleChild : rule->children)
        {
            if (ruleChild->background.has_value())
            {
                lineage->ruleBackground = ruleChild->background.value();
                UpdateSteps(ruleChild->background.value()->steps);
            }

            if (ruleChild->scenario.has_value())
            {
                UpdateScenario(ruleChild->scenario.value(), std::make_shared<Lineage>(*lineage + Lineage{ {}, {}, {}, rule }));
            }
        }
    }

    auto Query::UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, const std::shared_ptr<Lineage>& lineage) -> void
    {
        lineageById[scenario->id] = std::make_shared<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario });

        std::size_t examplesIndex = 0;
        for (const auto& examples : scenario->examples)
        {
            lineageById[examples->id] = std::make_shared<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario, examples, examplesIndex });

            std::size_t exampleIndex = 0;
            for (const auto& example : examples->tableBody)
            {
                lineageById[example->id] = std::make_shared<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario, examples, examplesIndex, example, exampleIndex });
                ++exampleIndex;
            }
            ++examplesIndex;
        }

        UpdateSteps(scenario->steps);
    }

    auto Query::UpdateSteps(const std::vector<std::shared_ptr<messages::Step>>& steps) -> void
    {
        for (const auto& step : steps)
        {
            stepById[step->id] = step;
        }
    }

    auto Query::UpdatePickle(std::shared_ptr<const messages::Pickle> pickle) -> void
    {
        auto&& entry = pickleById[pickle->id] = std::move(pickle);
        for (const auto& pickleStep : entry->steps)
        {
            pickleStepById[pickleStep->id] = pickleStep;
        }
    }

    auto Query::UpdateTestRunHookStarted(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted) -> void
    {
        testRunHookStartedById[testRunHookStarted->id] = testRunHookStarted;
    }

    auto Query::UpdateTestRunHookFinished(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) -> void
    {
        testRunHookFinishedByTestRunHookStartedId[testRunHookFinished->testRunHookStartedId] = testRunHookFinished;
    }

    auto Query::UpdateTestCase(std::shared_ptr<const messages::TestCase> testCase) -> void
    {
        for (const auto& testStep : testCase->testSteps)
        {
            testStepById[testStep->id] = testStep;
        }
        testCaseById[testCase->id] = std::move(testCase);
    }

    auto Query::UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted) -> void
    {
        testCaseStartedById[testCaseStarted->id] = std::move(testCaseStarted);
    }

    auto Query::UpdateAttachment(const std::shared_ptr<const messages::Attachment>& attachment) -> void
    {
        if (attachment->testCaseStartedId.has_value())
        {
            attachmentsByTestCaseStartedId[attachment->testCaseStartedId.value()].push_back(attachment);
        }
        if (attachment->testRunHookStartedId.has_value())
        {
            attachmentsByTestRunHookStartedId[attachment->testRunHookStartedId.value()].push_back(attachment);
        }
    }

    auto Query::UpdateTestStepFinished(std::shared_ptr<const messages::TestStepFinished> testStepFinished) -> void
    {
        testStepFinishedByTestCaseStartedId[testStepFinished->testCaseStartedId].push_back(std::move(testStepFinished));
    }

    auto Query::UpdateTestCaseFinished(std::shared_ptr<const messages::TestCaseFinished> testCaseFinished) -> void
    {
        testCaseFinishedByTestCaseStartedId[testCaseFinished->testCaseStartedId] = std::move(testCaseFinished);
    }
}
