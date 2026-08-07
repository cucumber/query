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
#include <variant>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        template<class T, typename Proj>
        void SortBy(std::vector<T>& container, const Proj& projection)
        {
            std::sort(container.begin(), container.end(),
                [&projection](const auto& lhs, const auto& rhs)
                {
                    return std::stoi(std::invoke(projection, lhs)) < std::stoi(std::invoke(projection, rhs));
                });
        }

        void SortBySeverity(std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>>& container)
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

    void Query::Update(const cucumber::messages::Envelope& envelope)
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
        // if (envelope.testRunStarted)
        // {
        //     this.testRunStarted = envelope.testRunStarted
        // }
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
        // if (envelope.testRunFinished)
        // {
        //     this.testRunFinished = envelope.testRunFinished
        // }
        if (envelope.suggestion.has_value())
        {
            suggestionsByPickleStepId[envelope.suggestion.value()->pickleStepId] = envelope.suggestion.value();
        }
        if (envelope.undefinedParameterType.has_value())
        {
            undefinedParameterTypes.push_back(envelope.undefinedParameterType.value());
        }
    }

    std::unordered_map<messages::TestStepResultStatus, std::size_t> Query::CountMostSevereTestStepResultStatus() const
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

    std::size_t Query::CountTestCasesStarted() const
    {
        return FindAllTestCaseStarted().size();
    }

    std::vector<std::shared_ptr<const messages::Pickle>> Query::FindAllPickles() const
    {
        return MapValuesToVectorSortBy(pickleById, &messages::Pickle::id);
    }

    std::vector<std::shared_ptr<const messages::PickleStep>> Query::FindAllPickleSteps() const
    {
        return MapValuesToVectorSortBy(pickleStepById, &messages::PickleStep::id);
    }

    std::vector<std::shared_ptr<const messages::StepDefinition>> Query::FindAllStepDefinitions() const
    {
        return MapValuesToVectorSortBy(stepDefinitionById, &messages::StepDefinition::id);
    }

    std::vector<std::shared_ptr<const messages::TestCaseStarted>> Query::FindAllTestCaseStarted() const
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

    std::vector<std::shared_ptr<const messages::TestCaseFinished>> Query::FindAllTestCaseFinished() const
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

    std::vector<std::shared_ptr<const messages::TestStep>> Query::FindAllTestSteps() const
    {
        return MapValuesToVectorSortBy(testStepById, &messages::TestStep::id);
    }

    std::vector<std::shared_ptr<const messages::TestCase>> Query::FindAllTestCases() const
    {
        return MapValuesToVectorSortBy(testCaseById, &messages::TestCase::id);
    }

    std::vector<std::shared_ptr<const messages::TestStepStarted>> Query::FindAllTestStepStarted() const
    {
        return MapValuesToVectorSortBy(testStepStartedByTestCaseStartedId, &messages::TestStepStarted::testCaseStartedId);
    }

    std::vector<std::shared_ptr<const messages::TestStepFinished>> Query::FindAllTestStepFinished() const
    {
        return MapValuesToVectorSortBy(testStepFinishedByTestCaseStartedId, &messages::TestStepFinished::testCaseStartedId);
    }

    std::vector<std::shared_ptr<const messages::TestRunHookStarted>> Query::FindAllTestRunHookStarted() const
    {
        return MapValuesToVectorSortBy(testRunHookStartedById, &messages::TestRunHookStarted::id);
    }

    std::vector<std::shared_ptr<const messages::TestRunHookFinished>> Query::FindAllTestRunHookFinished() const
    {
        return MapValuesToVectorSortBy(testRunHookFinishedByTestRunHookStartedId, &messages::TestRunHookFinished::testRunHookStartedId);
    }

    std::vector<std::shared_ptr<const messages::UndefinedParameterType>> Query::FindAllUndefinedParameterTypes() const
    {
        return undefinedParameterTypes;
    }

    std::vector<std::shared_ptr<const messages::Attachment>> Query::FindAttachmentsBy(
        std::variant<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestRunHookFinished>> element) const
    {
        return std::visit(
            overloaded{
                [this](const std::shared_ptr<const messages::TestStepFinished>& element)
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
                },
                [this](const std::shared_ptr<const messages::TestRunHookFinished>& element)
                {
                    if (attachmentsByTestRunHookStartedId.find(element->testRunHookStartedId) != attachmentsByTestRunHookStartedId.end())
                    {
                        return attachmentsByTestRunHookStartedId.at(element->testRunHookStartedId);
                    }
                    return std::vector<std::shared_ptr<const messages::Attachment>>{};
                },
            },
            element);
    }

    std::optional<std::shared_ptr<const messages::Hook>> Query::FindHookBy(
        std::variant<std::shared_ptr<const messages::TestStep>, std::shared_ptr<const messages::TestRunHookStarted>, std::shared_ptr<const messages::TestRunHookFinished>> element) const
    {
        return std::visit(
            overloaded{
                [this](const std::shared_ptr<const messages::TestStep>& element) -> std::optional<std::shared_ptr<const messages::Hook>>
                {
                    if (element->hookId.has_value())
                    {
                        return hookById.at(element->hookId.value());
                    }
                    return std::nullopt;
                },
                [this](const std::shared_ptr<const messages::TestRunHookStarted>& element) -> std::optional<std::shared_ptr<const messages::Hook>>
                {
                    if (hookById.find(element->hookId) != hookById.end())
                    {
                        return hookById.at(element->hookId);
                    }
                    return std::nullopt;
                },
                [this](const std::shared_ptr<const messages::TestRunHookFinished>& element) -> std::optional<std::shared_ptr<const messages::Hook>>
                {
                    const auto testRunHookStarted = FindTestRunHookStartedBy(element);
                    if (!testRunHookStarted.has_value())
                    {
                        throw std::out_of_range{ "Expected to find TestRunHookStarted from TestRunHookFinished" };
                    }
                    return FindHookBy(testRunHookStarted.value());
                },
            },
            element);
    }

    std::optional<std::shared_ptr<const messages::Meta>> Query::FindMeta() const
    {
        return meta;
    }

    std::optional<std::shared_ptr<const messages::TestStepResult>> Query::FindMostSevereTestStepResultBy(
        std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
    {
        const auto testCaseStarted = std::visit(overloaded{ [this](const std::shared_ptr<const messages::TestCaseFinished>& element)
                                                    {
                                                        return FindTestCaseStartedBy(element);
                                                    },
                                                    [](const auto& element) -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
                                                    {
                                                        return element;
                                                    } },
            element);

        if (testCaseStarted.has_value())
        {
            auto testStepFinishedAndTestStep = FindTestStepFinishedAndTestStepBy(testCaseStarted.value());
            if (!testStepFinishedAndTestStep.empty())
            {
                SortBySeverity(testStepFinishedAndTestStep);

                return testStepFinishedAndTestStep.front().first->testStepResult;
            }
        }

        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::Location>> Query::FindLocationOf(const std::shared_ptr<const messages::Pickle>& pickle) const
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

    std::optional<std::shared_ptr<const messages::Pickle>> Query::FindPickleBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
        std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>>
            element) const
    {
        const auto testCase = std::visit(
            [this](const auto& element)
            {
                return FindTestCaseBy(element);
            },
            element);

        try
        {
            return pickleById.at(testCase.value()->pickleId);
        }
        catch (const std::out_of_range&)
        {
            return std::nullopt;
        }
    }

    std::optional<std::shared_ptr<const messages::PickleStep>> Query::FindPickleStepBy(const std::shared_ptr<const messages::TestStep>& testStep) const
    {
        if (testStep->pickleStepId.has_value())
        {
            return pickleStepById.at(testStep->pickleStepId.value());
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::Step>> Query::FindStepBy(const std::shared_ptr<const messages::PickleStep>& pickleStep) const
    {
        const auto stepId = pickleStep->astNodeIds.front();

        if (stepById.find(stepId) != stepById.end())
        {
            return stepById.at(pickleStep->astNodeIds.front());
        }
        return std::nullopt;
    }

    std::vector<std::shared_ptr<const messages::StepDefinition>> Query::FindStepDefinitionsBy(const std::shared_ptr<const messages::TestStep>& testStep) const
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

    std::vector<std::shared_ptr<const messages::Suggestion>> Query::FindSuggestionsBy(const std::shared_ptr<const messages::PickleStep>& element) const
    {
        if (suggestionsByPickleStepId.find(element->id) != suggestionsByPickleStepId.end())
        {
            return { suggestionsByPickleStepId.at(element->id) };
        }

        return {};
    }

    std::vector<std::shared_ptr<const messages::Suggestion>> Query::FindSuggestionsBy(const std::shared_ptr<const messages::Pickle>& element) const
    {
        std::vector<std::shared_ptr<const messages::Suggestion>> result;
        for (const auto& pickleStep : element->steps)
        {
            const auto suggestions = FindSuggestionsBy(pickleStep);
            result.insert(result.end(), suggestions.begin(), suggestions.end());
        }
        return result;
    }

    std::optional<std::shared_ptr<const messages::TestCase>> Query::FindTestCaseBy(std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
        std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>>
            element) const
    {
        const auto testCaseStarted = std::visit(
            overloaded{
                [](std::shared_ptr<const messages::TestCaseStarted> element) -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
                {
                    return element;
                },
                [this](auto element) -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
                {
                    return FindTestCaseStartedBy(element);
                },
            },
            element);

        try
        {
            return testCaseById.at(testCaseStarted.value()->testCaseId);
        }
        catch (const std::out_of_range&)
        {
            return std::nullopt;
        }
    }

    std::optional<std::shared_ptr<const messages::Duration>> Query::FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseStarted>& element) const
    {
        const auto& testCaseFinished = FindTestCaseFinishedBy(element);
        if (testCaseFinished.has_value())
        {
            return std::make_shared<messages::Duration>(*testCaseFinished.value()->timestamp - *element->timestamp);
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::Duration>> Query::FindTestCaseDurationBy(const std::shared_ptr<const messages::TestCaseFinished>& element) const
    {
        const auto testCaseStarted = FindTestCaseStartedBy(element);

        if (testCaseStarted.has_value())
        {
            return FindTestCaseDurationBy(testCaseStarted.value());
        }

        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::TestCaseStarted>> Query::FindTestCaseStartedBy(
        std::variant<std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const
    {
        return std::visit(
            [this](const auto& item) -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
            {
                const auto iter = testCaseStartedById.find(item->testCaseStartedId);
                if (iter != testCaseStartedById.end())
                {
                    return iter->second;
                }
                return std::nullopt;
            },
            element);
    }

    std::optional<std::shared_ptr<const messages::TestCaseFinished>> Query::FindTestCaseFinishedBy(const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const
    {
        if (testCaseFinishedByTestCaseStartedId.find(testCaseStarted->id) != testCaseFinishedByTestCaseStartedId.end())
        {
            return testCaseFinishedByTestCaseStartedId.at(testCaseStarted->id);
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::TestRunHookStarted>> Query::FindTestRunHookStartedBy(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished) const
    {
        const auto iter = testRunHookStartedById.find(testRunHookFinished->testRunHookStartedId);
        if (iter != testRunHookStartedById.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<const messages::TestStep>> Query::FindTestStepBy(
        std::variant<std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>> element) const
    {
        try
        {
            return std::visit(
                [this](const auto& item)
                {
                    return testStepById.at(item->testStepId);
                },
                element);
        }
        catch (const std::out_of_range&)
        {
            return std::nullopt;
        }
    }

    std::vector<std::shared_ptr<const messages::TestStepFinished>> Query::FindTestStepsFinishedBy(
        std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
    {
        const auto& optionalTestCaseStarted = std::visit(overloaded{ [this](const std::shared_ptr<const messages::TestCaseFinished>& element)
                                                             {
                                                                 return FindTestCaseStartedBy(element);
                                                             },
                                                             [](const auto& element) -> std::optional<std::shared_ptr<const messages::TestCaseStarted>>
                                                             {
                                                                 return element;
                                                             } },
            element);

        if (optionalTestCaseStarted.has_value() && testStepFinishedByTestCaseStartedId.find(optionalTestCaseStarted.value()->id) != testStepFinishedByTestCaseStartedId.end())
        {
            return testStepFinishedByTestCaseStartedId.at(optionalTestCaseStarted.value()->id);
        }

        return {};
    }

    std::vector<std::pair<std::shared_ptr<const messages::TestStepFinished>, std::shared_ptr<const messages::TestStep>>> Query::FindTestStepFinishedAndTestStepBy(
        const std::shared_ptr<const messages::TestCaseStarted>& testCaseStarted) const
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

    auto Query::FindLineageBy(std::variant<std::shared_ptr<const messages::Pickle>, std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>> element) const
        -> std::optional<LineageAndPickle>
    {
        const auto pickle = std::visit(
            overloaded{
                [](const std::shared_ptr<const messages::Pickle>& pickle)
                {
                    return std::make_optional(pickle);
                },
                [this](const auto& element)
                {
                    return FindPickleBy(element);
                },
            },
            element);

        if (pickle.has_value())
        {
            return LineageAndPickle{ lineageById.at(pickle.value()->astNodeIds.back()), pickle.value() };
        }

        return std::nullopt;
    }

    void Query::UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument)
    {
        if (gherkinDocument->feature.has_value())
        {
            UpdateFeature(gherkinDocument->feature.value(), std::make_shared<Lineage>(Lineage{ gherkinDocument }));
        }
    }

    void Query::UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, const std::shared_ptr<Lineage>& lineage)
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

    void Query::UpdateRule(const std::shared_ptr<const messages::Rule>& rule, const std::shared_ptr<Lineage>& lineage)
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

    void Query::UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, const std::shared_ptr<Lineage>& lineage)
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

    void Query::UpdateSteps(const std::vector<std::shared_ptr<messages::Step>>& steps)
    {
        for (const auto& step : steps)
        {
            stepById[step->id] = step;
        }
    }

    void Query::UpdatePickle(std::shared_ptr<const messages::Pickle> pickle)
    {
        auto&& entry = pickleById[pickle->id] = std::move(pickle);
        for (const auto& pickleStep : entry->steps)
        {
            pickleStepById[pickleStep->id] = pickleStep;
        }
    }

    void Query::UpdateTestRunHookStarted(const std::shared_ptr<const messages::TestRunHookStarted>& testRunHookStarted)
    {
        testRunHookStartedById[testRunHookStarted->id] = testRunHookStarted;
    }

    void Query::UpdateTestRunHookFinished(const std::shared_ptr<const messages::TestRunHookFinished>& testRunHookFinished)
    {
        testRunHookFinishedByTestRunHookStartedId[testRunHookFinished->testRunHookStartedId] = testRunHookFinished;
    }

    /////////////////////////////
    /////////////////////////////
    /////////////////////////////

    void Query::UpdateTestCase(std::shared_ptr<const messages::TestCase> testCase)
    {
        for (const auto& testStep : testCase->testSteps)
        {
            testStepById[testStep->id] = testStep;
        }
        testCaseById[testCase->id] = std::move(testCase);
    }

    void Query::UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted)
    {
        testCaseStartedById[testCaseStarted->id] = std::move(testCaseStarted);
    }

    /////////////////////////////

    void Query::UpdateAttachment(const std::shared_ptr<const messages::Attachment>& attachment)
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

    void Query::UpdateTestStepFinished(std::shared_ptr<const messages::TestStepFinished> testStepFinished)
    {
        testStepFinishedByTestCaseStartedId[testStepFinished->testCaseStartedId].push_back(std::move(testStepFinished));
    }

    void Query::UpdateTestCaseFinished(std::shared_ptr<const messages::TestCaseFinished> testCaseFinished)
    {
        testCaseFinishedByTestCaseStartedId[testCaseFinished->testCaseStartedId] = std::move(testCaseFinished);
    }

    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
}
