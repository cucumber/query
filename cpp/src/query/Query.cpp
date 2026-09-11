#include "cucumber/query/Query.hpp"
#include "cucumber/messages/All.hpp"
#include "cucumber/messages/DurationUtil.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/Lineage.hpp"
#include "cucumber/query/View.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        auto SortBySeverity(std::vector<TestStepFinishedAndTestStep>& container) -> void
        {
            std::sort(container.begin(), container.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    using underlying_type = std::underlying_type_t<messages::TestStepResultStatus>;
                    return static_cast<underlying_type>(lhs.testStepFinished->testStepResult.status) > static_cast<underlying_type>(rhs.testStepFinished->testStepResult.status);
                });
        }

        template<typename T>
        auto Empty() -> const Pointers<T>&
        {
            static const Pointers<T> empty;
            return empty;
        }

        template<typename T>
        auto FindOne(const ById<T>& container, const std::string& id) -> const T*
        {
            const auto iter = container.find(id);
            return iter != container.end() ? iter->second : nullptr;
        }

        auto FindOne(const std::map<std::string, Lineage, StringIdCompare>& container, const std::string& id) -> const Lineage*
        {
            const auto iter = container.find(id);
            return iter != container.end() ? &iter->second : nullptr;
        }

        template<typename T>
        auto FindMany(const ManyById<T>& container, const std::string& id) -> const Pointers<T>&
        {
            const auto iter = container.find(id);
            return iter != container.end() ? iter->second : Empty<T>();
        }
    }

    auto Query::Update(const std::shared_ptr<const cucumber::messages::Envelope>& envelope) -> void
    {
        Update(*envelope);
    }

    auto Query::Update(const cucumber::messages::Envelope& envelope) -> void
    {
        if (envelope.meta)
        {
            meta = std::addressof(*envelope.meta);
        }
        if (envelope.gherkinDocument)
        {
            UpdateGherkinDocument(*envelope.gherkinDocument);
        }
        if (envelope.pickle)
        {
            UpdatePickle(*envelope.pickle);
        }
        if (envelope.hook)
        {
            hookById[envelope.hook->id] = std::addressof(*envelope.hook);
        }
        if (envelope.stepDefinition)
        {
            stepDefinitionById[envelope.stepDefinition->id] = std::addressof(*envelope.stepDefinition);
        }
        if (envelope.testRunStarted)
        {
            testRunStarted = std::addressof(*envelope.testRunStarted);
        }
        if (envelope.testRunHookStarted)
        {
            UpdateTestRunHookStarted(*envelope.testRunHookStarted);
        }
        if (envelope.testRunHookFinished)
        {
            UpdateTestRunHookFinished(*envelope.testRunHookFinished);
        }
        if (envelope.testCase)
        {
            UpdateTestCase(*envelope.testCase);
        }
        if (envelope.testCaseStarted)
        {
            UpdateTestCaseStarted(*envelope.testCaseStarted);
        }
        if (envelope.testStepStarted)
        {
            testStepStartedByTestCaseStartedId[envelope.testStepStarted->testCaseStartedId].push_back(std::addressof(*envelope.testStepStarted));
        }
        if (envelope.attachment)
        {
            UpdateAttachment(*envelope.attachment);
        }
        if (envelope.testStepFinished)
        {
            UpdateTestStepFinished(*envelope.testStepFinished);
        }
        if (envelope.testCaseFinished)
        {
            UpdateTestCaseFinished(*envelope.testCaseFinished);
        }
        if (envelope.testRunFinished)
        {
            testRunFinished = std::addressof(*envelope.testRunFinished);
        }
        if (envelope.suggestion)
        {
            suggestionsByPickleStepId[envelope.suggestion->pickleStepId] = std::addressof(*envelope.suggestion);
        }
        if (envelope.undefinedParameterType)
        {
            undefinedParameterTypes.push_back(std::addressof(*envelope.undefinedParameterType));
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

                ++result[testStepFinishedAndTestStep.front().testStepFinished->testStepResult.status];
            }
        }

        return result;
    }

    auto Query::CountTestCasesStarted() const -> std::size_t
    {
        return FindAllTestCaseStarted().size();
    }

    auto Query::FindAllPickles() const -> ValuesView<messages::Pickle>
    {
        return pickleById | views::Values() | views::Dereference();
    }

    auto Query::FindAllPickleSteps() const -> ValuesView<messages::PickleStep>
    {
        return pickleStepById | views::Values() | views::Dereference();
    }

    auto Query::FindAllStepDefinitions() const -> ValuesView<messages::StepDefinition>
    {
        return stepDefinitionById | views::Values() | views::Dereference();
    }

    auto Query::FindAllTestCaseStarted() const -> FilteredValuesView<messages::TestCaseStarted>
    {
        return testCaseStartedById | views::Values() | views::Dereference() |
               views::Filter(Predicate<messages::TestCaseStarted>{ [this](const messages::TestCaseStarted& testCaseStarted)
                   {
                       const auto* testCaseFinished = FindTestCaseFinishedBy(testCaseStarted);
                       return testCaseFinished == nullptr || !testCaseFinished->willBeRetried;
                   } });
    }

    auto Query::FindAllTestCaseFinished() const -> FilteredValuesView<messages::TestCaseFinished>
    {
        return testCaseFinishedByTestCaseStartedId | views::Values() | views::Dereference() |
               views::Filter(Predicate<messages::TestCaseFinished>{ [](const messages::TestCaseFinished& testCaseFinished)
                   {
                       return !testCaseFinished.willBeRetried;
                   } });
    }

    auto Query::FindAllTestSteps() const -> ValuesView<messages::TestStep>
    {
        return testStepById | views::Values() | views::Dereference();
    }

    auto Query::FindAllTestCases() const -> ValuesView<messages::TestCase>
    {
        return testCaseById | views::Values() | views::Dereference();
    }

    auto Query::FindAllTestStepStarted() const -> JoinedValuesView<messages::TestStepStarted>
    {
        return testStepStartedByTestCaseStartedId | views::Values() | views::Join() | views::Dereference();
    }

    auto Query::FindAllTestStepFinished() const -> JoinedValuesView<messages::TestStepFinished>
    {
        return testStepFinishedByTestCaseStartedId | views::Values() | views::Join() | views::Dereference();
    }

    auto Query::FindAllTestRunHookStarted() const -> ValuesView<messages::TestRunHookStarted>
    {
        return testRunHookStartedById | views::Values() | views::Dereference();
    }

    auto Query::FindAllTestRunHookFinished() const -> ValuesView<messages::TestRunHookFinished>
    {
        return testRunHookFinishedByTestRunHookStartedId | views::Values() | views::Dereference();
    }

    auto Query::FindAllUndefinedParameterTypes() const -> ElementsView<messages::UndefinedParameterType>
    {
        return undefinedParameterTypes | views::Dereference();
    }

    auto Query::FindAttachmentsBy(const messages::TestStepFinished& element) const -> FilteredElementsView<messages::Attachment>
    {
        return FindMany(attachmentsByTestCaseStartedId, element.testCaseStartedId) | views::Dereference() |
               views::Filter(Predicate<messages::Attachment>{ [testStepId = element.testStepId](const messages::Attachment& attachment)
                   {
                       return attachment.testStepId == testStepId;
                   } });
    }

    auto Query::FindAttachmentsBy(const messages::TestRunHookFinished& element) const -> ElementsView<messages::Attachment>
    {
        return FindMany(attachmentsByTestRunHookStartedId, element.testRunHookStartedId) | views::Dereference();
    }

    auto Query::FindHookBy(const messages::TestStep& element) const -> const messages::Hook*
    {
        if (element.hookId)
        {
            return FindOne(hookById, *element.hookId);
        }
        return nullptr;
    }

    auto Query::FindHookBy(const messages::TestRunHookStarted& element) const -> const messages::Hook*
    {
        return FindOne(hookById, element.hookId);
    }

    auto Query::FindHookBy(const messages::TestRunHookFinished& element) const -> const messages::Hook*
    {
        const auto* testRunHookStarted = FindTestRunHookStartedBy(element);
        if (testRunHookStarted != nullptr)
        {
            return FindHookBy(*testRunHookStarted);
        }
        return nullptr;
    }

    auto Query::FindMeta() const -> const messages::Meta*
    {
        return meta;
    }

    auto Query::FindMostSevereTestStepResultBy(const messages::TestCaseStarted& element) const -> const messages::TestStepResult*
    {
        auto testStepFinishedAndTestStep = FindTestStepFinishedAndTestStepBy(element);
        if (!testStepFinishedAndTestStep.empty())
        {
            SortBySeverity(testStepFinishedAndTestStep);

            return std::addressof(testStepFinishedAndTestStep.front().testStepFinished->testStepResult);
        }
        return nullptr;
    }

    auto Query::FindMostSevereTestStepResultBy(const messages::TestCaseFinished& element) const -> const messages::TestStepResult*
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindMostSevereTestStepResultBy(*testCaseStarted);
        }
        return nullptr;
    }

    auto Query::FindLocationOf(const messages::Pickle& pickle) const -> const messages::Location*
    {
        if (pickle.location)
        {
            return std::addressof(*pickle.location);
        }

        const auto lineageAndPickle = FindLineageBy(pickle);

        if (lineageAndPickle)
        {
            const auto* lineage = lineageAndPickle->lineage;

            if (lineage->example)
            {
                return std::addressof(lineage->example->location);
            }

            if (lineage->scenario)
            {
                return std::addressof(lineage->scenario->location);
            }
        }

        return nullptr;
    }

    auto Query::FindPickleBy(const messages::TestCaseStarted& element) const -> const messages::Pickle*
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    auto Query::FindPickleBy(const messages::TestCaseFinished& element) const -> const messages::Pickle*
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    auto Query::FindPickleBy(const messages::TestStepStarted& element) const -> const messages::Pickle*
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    auto Query::FindPickleBy(const messages::TestStepFinished& element) const -> const messages::Pickle*
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    auto Query::FindPickleStepBy(const messages::TestStep& testStep) const -> const messages::PickleStep*
    {
        if (testStep.pickleStepId)
        {
            return FindOne(pickleStepById, *testStep.pickleStepId);
        }
        return nullptr;
    }

    auto Query::FindStepBy(const messages::PickleStep& pickleStep) const -> const messages::Step*
    {
        return FindOne(stepById, pickleStep.astNodeIds.front());
    }

    auto Query::FindStepDefinitionsBy(const messages::TestStep& testStep) const -> OwningView<messages::StepDefinition>
    {
        std::vector<const messages::StepDefinition*> result;

        if (testStep.stepDefinitionIds)
        {
            for (const auto& stepDefinitionId : *testStep.stepDefinitionIds)
            {
                const auto* stepDefinition = FindOne(stepDefinitionById, stepDefinitionId);
                if (stepDefinition != nullptr)
                {
                    result.push_back(stepDefinition);
                }
            }
        }

        return OwningView<messages::StepDefinition>{ std::move(result), views::SelectPointee{} };
    }

    auto Query::FindSuggestionsBy(const messages::PickleStep& element) const -> OwningView<messages::Suggestion>
    {
        std::vector<const messages::Suggestion*> result;

        const auto* suggestion = FindOne(suggestionsByPickleStepId, element.id);
        if (suggestion != nullptr)
        {
            result.push_back(suggestion);
        }

        return OwningView<messages::Suggestion>{ std::move(result), views::SelectPointee{} };
    }

    auto Query::FindSuggestionsBy(const messages::Pickle& element) const -> OwningView<messages::Suggestion>
    {
        std::vector<const messages::Suggestion*> result;

        for (const auto& pickleStep : element.steps)
        {
            const auto* suggestion = FindOne(suggestionsByPickleStepId, pickleStep.id);
            if (suggestion != nullptr)
            {
                result.push_back(suggestion);
            }
        }

        return OwningView<messages::Suggestion>{ std::move(result), views::SelectPointee{} };
    }

    auto Query::FindUnambiguousStepDefinitionBy(const messages::TestStep& testStep) const -> const messages::StepDefinition*
    {
        if (testStep.stepDefinitionIds && testStep.stepDefinitionIds->size() == 1)
        {
            return FindOne(stepDefinitionById, testStep.stepDefinitionIds->front());
        }
        return nullptr;
    }

    auto Query::FindTestCaseBy(const messages::TestCaseStarted& element) const -> const messages::TestCase*
    {
        return FindOne(testCaseById, element.testCaseId);
    }

    auto Query::FindTestCaseBy(const messages::TestCaseFinished& element) const -> const messages::TestCase*
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    auto Query::FindTestCaseBy(const messages::TestStepStarted& element) const -> const messages::TestCase*
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    auto Query::FindTestCaseBy(const messages::TestStepFinished& element) const -> const messages::TestCase*
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    auto Query::FindTestCaseDurationBy(const messages::TestCaseStarted& element) const -> std::optional<messages::Duration>
    {
        const auto* testCaseFinished = FindTestCaseFinishedBy(element);
        if (testCaseFinished != nullptr)
        {
            return testCaseFinished->timestamp - element.timestamp;
        }
        return std::nullopt;
    }

    auto Query::FindTestCaseDurationBy(const messages::TestCaseFinished& element) const -> std::optional<messages::Duration>
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);

        if (testCaseStarted != nullptr)
        {
            return FindTestCaseDurationBy(*testCaseStarted);
        }

        return std::nullopt;
    }

    auto Query::FindTestCaseStartedBy(const messages::TestCaseFinished& element) const -> const messages::TestCaseStarted*
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    auto Query::FindTestCaseStartedBy(const messages::TestStepStarted& element) const -> const messages::TestCaseStarted*
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    auto Query::FindTestCaseStartedBy(const messages::TestStepFinished& element) const -> const messages::TestCaseStarted*
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    auto Query::FindTestCaseFinishedBy(const messages::TestCaseStarted& testCaseStarted) const -> const messages::TestCaseFinished*
    {
        return FindOne(testCaseFinishedByTestCaseStartedId, testCaseStarted.id);
    }

    auto Query::FindTestRunHookStartedBy(const messages::TestRunHookFinished& testRunHookFinished) const -> const messages::TestRunHookStarted*
    {
        return FindOne(testRunHookStartedById, testRunHookFinished.testRunHookStartedId);
    }

    auto Query::FindTestRunHookFinishedBy(const messages::TestRunHookStarted& testRunHookStarted) const -> const messages::TestRunHookFinished*
    {
        return FindOne(testRunHookFinishedByTestRunHookStartedId, testRunHookStarted.id);
    }

    auto Query::FindTestRunDuration() const -> std::optional<messages::Duration>
    {
        if (testRunStarted != nullptr && testRunFinished != nullptr)
        {
            return testRunFinished->timestamp - testRunStarted->timestamp;
        }

        return std::nullopt;
    }

    auto Query::FindTestRunFinished() const -> const messages::TestRunFinished*
    {
        return testRunFinished;
    }

    auto Query::FindTestRunStarted() const -> const messages::TestRunStarted*
    {
        return testRunStarted;
    }

    auto Query::FindTestStepBy(const messages::TestStepStarted& element) const -> const messages::TestStep*
    {
        return FindOne(testStepById, element.testStepId);
    }

    auto Query::FindTestStepBy(const messages::TestStepFinished& element) const -> const messages::TestStep*
    {
        return FindOne(testStepById, element.testStepId);
    }

    auto Query::FindTestStepsStartedBy(const messages::TestCaseStarted& testCaseStarted) const -> ElementsView<messages::TestStepStarted>
    {
        return FindMany(testStepStartedByTestCaseStartedId, testCaseStarted.id) | views::Dereference();
    }

    auto Query::FindTestStepsStartedBy(const messages::TestCaseFinished& testCaseFinished) const -> ElementsView<messages::TestStepStarted>
    {
        return FindMany(testStepStartedByTestCaseStartedId, testCaseFinished.testCaseStartedId) | views::Dereference();
    }

    auto Query::FindTestStepsFinishedBy(const messages::TestCaseStarted& element) const -> ElementsView<messages::TestStepFinished>
    {
        return FindMany(testStepFinishedByTestCaseStartedId, element.id) | views::Dereference();
    }

    auto Query::FindTestStepsFinishedBy(const messages::TestCaseFinished& element) const -> ElementsView<messages::TestStepFinished>
    {
        return FindMany(testStepFinishedByTestCaseStartedId, element.testCaseStartedId) | views::Dereference();
    }

    auto Query::FindTestStepFinishedAndTestStepBy(const messages::TestCaseStarted& testCaseStarted) const -> std::vector<TestStepFinishedAndTestStep>
    {
        std::vector<TestStepFinishedAndTestStep> result;

        for (const auto& testStepFinished : FindTestStepsFinishedBy(testCaseStarted))
        {
            const auto* testStep = FindTestStepBy(testStepFinished);
            if (testStep != nullptr)
            {
                result.emplace_back(TestStepFinishedAndTestStep{ &testStepFinished, testStep });
            }
        }

        return result;
    }

    auto Query::FindLineageBy(const messages::Pickle& element) const -> std::optional<LineageAndPickle>
    {
        const auto* lineage = FindOne(lineageById, element.astNodeIds.back());

        if (lineage != nullptr)
        {
            return LineageAndPickle{ lineage, &element };
        }

        return std::nullopt;
    }

    auto Query::FindLineageBy(const messages::TestCaseStarted& element) const -> std::optional<LineageAndPickle>
    {
        const auto* pickle = FindPickleBy(element);

        if (pickle != nullptr)
        {
            return FindLineageBy(*pickle);
        }

        return std::nullopt;
    }

    auto Query::FindLineageBy(const messages::TestCaseFinished& element) const -> std::optional<LineageAndPickle>
    {
        const auto* pickle = FindPickleBy(element);

        if (pickle != nullptr)
        {
            return FindLineageBy(*pickle);
        }

        return std::nullopt;
    }

    auto Query::AllTestCaseStarted() const -> std::vector<const messages::TestCaseStarted*>
    {
        std::vector<const messages::TestCaseStarted*> result;

        for (const auto& testCaseStarted : FindAllTestCaseStarted())
        {
            result.push_back(&testCaseStarted);
        }

        return result;
    }

    auto Query::AllTestCaseFinished() const -> std::vector<const messages::TestCaseFinished*>
    {
        std::vector<const messages::TestCaseFinished*> result;

        for (const auto& testCaseFinished : FindAllTestCaseFinished())
        {
            result.push_back(&testCaseFinished);
        }

        return result;
    }

    auto Query::UpdateGherkinDocument(const messages::GherkinDocument& gherkinDocument) -> void
    {
        if (gherkinDocument.feature)
        {
            UpdateFeature(*gherkinDocument.feature, Lineage{ &gherkinDocument });
        }
    }

    auto Query::UpdateFeature(const messages::Feature& feature, Lineage lineage) -> void
    {
        for (const auto& featureChild : feature.children)
        {
            if (featureChild.background)
            {
                lineage.background = std::addressof(*featureChild.background);
                UpdateSteps(featureChild.background->steps);
            }

            if (featureChild.scenario)
            {
                UpdateScenario(*featureChild.scenario, lineage + Lineage{ nullptr, &feature });
            }

            if (featureChild.rule)
            {
                UpdateRule(*featureChild.rule, lineage + Lineage{ nullptr, &feature });
            }
        }
    }

    auto Query::UpdateRule(const messages::Rule& rule, Lineage lineage) -> void
    {
        for (const auto& ruleChild : rule.children)
        {
            if (ruleChild.background)
            {
                lineage.ruleBackground = std::addressof(*ruleChild.background);
                UpdateSteps(ruleChild.background->steps);
            }

            if (ruleChild.scenario)
            {
                UpdateScenario(*ruleChild.scenario, lineage + Lineage{ nullptr, nullptr, nullptr, &rule });
            }
        }
    }

    auto Query::UpdateScenario(const messages::Scenario& scenario, const Lineage& lineage) -> void
    {
        lineageById[scenario.id] = lineage + Lineage{ nullptr, nullptr, nullptr, nullptr, nullptr, &scenario };

        std::size_t examplesIndex = 0;
        for (const auto& examples : scenario.examples)
        {
            lineageById[examples.id] = lineage + Lineage{ nullptr, nullptr, nullptr, nullptr, nullptr, &scenario, std::addressof(examples), examplesIndex };

            std::size_t exampleIndex = 0;
            for (const auto& example : examples.tableBody)
            {
                lineageById[example.id] = lineage + Lineage{ nullptr, nullptr, nullptr, nullptr, nullptr, &scenario, std::addressof(examples), examplesIndex, std::addressof(example), exampleIndex };
                ++exampleIndex;
            }
            ++examplesIndex;
        }

        UpdateSteps(scenario.steps);
    }

    auto Query::UpdateSteps(const std::vector<messages::Step>& steps) -> void
    {
        for (const auto& step : steps)
        {
            stepById[step.id] = std::addressof(step);
        }
    }

    auto Query::UpdatePickle(const messages::Pickle& pickle) -> void
    {
        pickleById[pickle.id] = &pickle;
        for (const auto& pickleStep : pickle.steps)
        {
            pickleStepById[pickleStep.id] = std::addressof(pickleStep);
        }
    }

    auto Query::UpdateTestRunHookStarted(const messages::TestRunHookStarted& testRunHookStarted) -> void
    {
        testRunHookStartedById[testRunHookStarted.id] = &testRunHookStarted;
    }

    auto Query::UpdateTestRunHookFinished(const messages::TestRunHookFinished& testRunHookFinished) -> void
    {
        testRunHookFinishedByTestRunHookStartedId[testRunHookFinished.testRunHookStartedId] = &testRunHookFinished;
    }

    auto Query::UpdateTestCase(const messages::TestCase& testCase) -> void
    {
        for (const auto& testStep : testCase.testSteps)
        {
            testStepById[testStep.id] = std::addressof(testStep);
        }
        testCaseById[testCase.id] = &testCase;
    }

    auto Query::UpdateTestCaseStarted(const messages::TestCaseStarted& testCaseStarted) -> void
    {
        testCaseStartedById[testCaseStarted.id] = &testCaseStarted;
    }

    auto Query::UpdateAttachment(const messages::Attachment& attachment) -> void
    {
        if (attachment.testCaseStartedId)
        {
            attachmentsByTestCaseStartedId[*attachment.testCaseStartedId].push_back(&attachment);
        }
        if (attachment.testRunHookStartedId)
        {
            attachmentsByTestRunHookStartedId[*attachment.testRunHookStartedId].push_back(&attachment);
        }
    }

    auto Query::UpdateTestStepFinished(const messages::TestStepFinished& testStepFinished) -> void
    {
        testStepFinishedByTestCaseStartedId[testStepFinished.testCaseStartedId].push_back(&testStepFinished);
    }

    auto Query::UpdateTestCaseFinished(const messages::TestCaseFinished& testCaseFinished) -> void
    {
        testCaseFinishedByTestCaseStartedId[testCaseFinished.testCaseStartedId] = &testCaseFinished;
    }
}
