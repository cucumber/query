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
        void SortBySeverity(std::vector<TestStepFinishedAndTestStep>& container)
        {
            std::sort(container.begin(), container.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    using underlying_type = std::underlying_type_t<messages::TestStepResultStatus>;
                    return static_cast<underlying_type>(lhs.testStepFinished->testStepResult.status) > static_cast<underlying_type>(rhs.testStepFinished->testStepResult.status);
                });
        }

        template<typename T>
        const Pointers<T>& Empty()
        {
            static const Pointers<T> empty;
            return empty;
        }

        template<typename T>
        const T* FindOne(const ById<T>& container, const std::string& id)
        {
            const auto iter = container.find(id);
            return iter != container.end() ? iter->second : nullptr;
        }

        const Lineage* FindOne(const std::map<std::string, Lineage, StringIdCompare>& container, const std::string& id)
        {
            const auto iter = container.find(id);
            return iter != container.end() ? &iter->second : nullptr;
        }

        template<typename T>
        const Pointers<T>& FindMany(const ManyById<T>& container, const std::string& id)
        {
            const auto iter = container.find(id);
            return iter != container.end() ? iter->second : Empty<T>();
        }
    }

    void Query::Update(const std::shared_ptr<const cucumber::messages::Envelope>& envelope)
    {
        Update(*envelope);
    }

    void Query::Update(const cucumber::messages::Envelope& envelope)
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

                ++result[testStepFinishedAndTestStep.front().testStepFinished->testStepResult.status];
            }
        }

        return result;
    }

    std::size_t Query::CountTestCasesStarted() const
    {
        return FindAllTestCaseStarted().size();
    }

    ValuesView<messages::Pickle> Query::FindAllPickles() const
    {
        return pickleById | views::Values() | views::Dereference();
    }

    ValuesView<messages::PickleStep> Query::FindAllPickleSteps() const
    {
        return pickleStepById | views::Values() | views::Dereference();
    }

    ValuesView<messages::StepDefinition> Query::FindAllStepDefinitions() const
    {
        return stepDefinitionById | views::Values() | views::Dereference();
    }

    FilteredValuesView<messages::TestCaseStarted> Query::FindAllTestCaseStarted() const
    {
        return testCaseStartedById | views::Values() | views::Dereference() |
               views::Filter(Predicate<messages::TestCaseStarted>{ [this](const messages::TestCaseStarted& testCaseStarted)
                   {
                       const auto* testCaseFinished = FindTestCaseFinishedBy(testCaseStarted);
                       return testCaseFinished == nullptr || !testCaseFinished->willBeRetried;
                   } });
    }

    FilteredValuesView<messages::TestCaseFinished> Query::FindAllTestCaseFinished() const
    {
        return testCaseFinishedByTestCaseStartedId | views::Values() | views::Dereference() |
               views::Filter(Predicate<messages::TestCaseFinished>{ [](const messages::TestCaseFinished& testCaseFinished)
                   {
                       return !testCaseFinished.willBeRetried;
                   } });
    }

    ValuesView<messages::TestStep> Query::FindAllTestSteps() const
    {
        return testStepById | views::Values() | views::Dereference();
    }

    ValuesView<messages::TestCase> Query::FindAllTestCases() const
    {
        return testCaseById | views::Values() | views::Dereference();
    }

    JoinedValuesView<messages::TestStepStarted> Query::FindAllTestStepStarted() const
    {
        return testStepStartedByTestCaseStartedId | views::Values() | views::Join() | views::Dereference();
    }

    JoinedValuesView<messages::TestStepFinished> Query::FindAllTestStepFinished() const
    {
        return testStepFinishedByTestCaseStartedId | views::Values() | views::Join() | views::Dereference();
    }

    ValuesView<messages::TestRunHookStarted> Query::FindAllTestRunHookStarted() const
    {
        return testRunHookStartedById | views::Values() | views::Dereference();
    }

    ValuesView<messages::TestRunHookFinished> Query::FindAllTestRunHookFinished() const
    {
        return testRunHookFinishedByTestRunHookStartedId | views::Values() | views::Dereference();
    }

    ElementsView<messages::UndefinedParameterType> Query::FindAllUndefinedParameterTypes() const
    {
        return undefinedParameterTypes | views::Dereference();
    }

    FilteredElementsView<messages::Attachment> Query::FindAttachmentsBy(const messages::TestStepFinished& element) const
    {
        return FindMany(attachmentsByTestCaseStartedId, element.testCaseStartedId) | views::Dereference() |
               views::Filter(Predicate<messages::Attachment>{ [testStepId = element.testStepId](const messages::Attachment& attachment)
                   {
                       return attachment.testStepId == testStepId;
                   } });
    }

    ElementsView<messages::Attachment> Query::FindAttachmentsBy(const messages::TestRunHookFinished& element) const
    {
        return FindMany(attachmentsByTestRunHookStartedId, element.testRunHookStartedId) | views::Dereference();
    }

    const messages::Hook* Query::FindHookBy(const messages::TestStep& element) const
    {
        if (element.hookId)
        {
            return FindOne(hookById, *element.hookId);
        }
        return nullptr;
    }

    const messages::Hook* Query::FindHookBy(const messages::TestRunHookStarted& element) const
    {
        return FindOne(hookById, element.hookId);
    }

    const messages::Hook* Query::FindHookBy(const messages::TestRunHookFinished& element) const
    {
        const auto* testRunHookStarted = FindTestRunHookStartedBy(element);
        if (testRunHookStarted != nullptr)
        {
            return FindHookBy(*testRunHookStarted);
        }
        return nullptr;
    }

    const messages::Meta* Query::FindMeta() const
    {
        return meta;
    }

    const messages::TestStepResult* Query::FindMostSevereTestStepResultBy(const messages::TestCaseStarted& element) const
    {
        auto testStepFinishedAndTestStep = FindTestStepFinishedAndTestStepBy(element);
        if (!testStepFinishedAndTestStep.empty())
        {
            SortBySeverity(testStepFinishedAndTestStep);

            return std::addressof(testStepFinishedAndTestStep.front().testStepFinished->testStepResult);
        }
        return nullptr;
    }

    const messages::TestStepResult* Query::FindMostSevereTestStepResultBy(const messages::TestCaseFinished& element) const
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindMostSevereTestStepResultBy(*testCaseStarted);
        }
        return nullptr;
    }

    const messages::Location* Query::FindLocationOf(const messages::Pickle& pickle) const
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

    const messages::Pickle* Query::FindPickleBy(const messages::TestCaseStarted& element) const
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    const messages::Pickle* Query::FindPickleBy(const messages::TestCaseFinished& element) const
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    const messages::Pickle* Query::FindPickleBy(const messages::TestStepStarted& element) const
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    const messages::Pickle* Query::FindPickleBy(const messages::TestStepFinished& element) const
    {
        const auto* testCase = FindTestCaseBy(element);
        if (testCase != nullptr)
        {
            return FindOne(pickleById, testCase->pickleId);
        }
        return nullptr;
    }

    const messages::PickleStep* Query::FindPickleStepBy(const messages::TestStep& testStep) const
    {
        if (testStep.pickleStepId)
        {
            return FindOne(pickleStepById, *testStep.pickleStepId);
        }
        return nullptr;
    }

    const messages::Step* Query::FindStepBy(const messages::PickleStep& pickleStep) const
    {
        return FindOne(stepById, pickleStep.astNodeIds.front());
    }

    OwningView<messages::StepDefinition> Query::FindStepDefinitionsBy(const messages::TestStep& testStep) const
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

    OwningView<messages::Suggestion> Query::FindSuggestionsBy(const messages::PickleStep& element) const
    {
        std::vector<const messages::Suggestion*> result;

        const auto* suggestion = FindOne(suggestionsByPickleStepId, element.id);
        if (suggestion != nullptr)
        {
            result.push_back(suggestion);
        }

        return OwningView<messages::Suggestion>{ std::move(result), views::SelectPointee{} };
    }

    OwningView<messages::Suggestion> Query::FindSuggestionsBy(const messages::Pickle& element) const
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

    const messages::StepDefinition* Query::FindUnambiguousStepDefinitionBy(const messages::TestStep& testStep) const
    {
        if (testStep.stepDefinitionIds && testStep.stepDefinitionIds->size() == 1)
        {
            return FindOne(stepDefinitionById, testStep.stepDefinitionIds->front());
        }
        return nullptr;
    }

    const messages::TestCase* Query::FindTestCaseBy(const messages::TestCaseStarted& element) const
    {
        return FindOne(testCaseById, element.testCaseId);
    }

    const messages::TestCase* Query::FindTestCaseBy(const messages::TestCaseFinished& element) const
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    const messages::TestCase* Query::FindTestCaseBy(const messages::TestStepStarted& element) const
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    const messages::TestCase* Query::FindTestCaseBy(const messages::TestStepFinished& element) const
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);
        if (testCaseStarted != nullptr)
        {
            return FindTestCaseBy(*testCaseStarted);
        }
        return nullptr;
    }

    std::optional<messages::Duration> Query::FindTestCaseDurationBy(const messages::TestCaseStarted& element) const
    {
        const auto* testCaseFinished = FindTestCaseFinishedBy(element);
        if (testCaseFinished != nullptr)
        {
            return testCaseFinished->timestamp - element.timestamp;
        }
        return std::nullopt;
    }

    std::optional<messages::Duration> Query::FindTestCaseDurationBy(const messages::TestCaseFinished& element) const
    {
        const auto* testCaseStarted = FindTestCaseStartedBy(element);

        if (testCaseStarted != nullptr)
        {
            return FindTestCaseDurationBy(*testCaseStarted);
        }

        return std::nullopt;
    }

    const messages::TestCaseStarted* Query::FindTestCaseStartedBy(const messages::TestCaseFinished& element) const
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    const messages::TestCaseStarted* Query::FindTestCaseStartedBy(const messages::TestStepStarted& element) const
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    const messages::TestCaseStarted* Query::FindTestCaseStartedBy(const messages::TestStepFinished& element) const
    {
        return FindOne(testCaseStartedById, element.testCaseStartedId);
    }

    const messages::TestCaseFinished* Query::FindTestCaseFinishedBy(const messages::TestCaseStarted& testCaseStarted) const
    {
        return FindOne(testCaseFinishedByTestCaseStartedId, testCaseStarted.id);
    }

    const messages::TestRunHookStarted* Query::FindTestRunHookStartedBy(const messages::TestRunHookFinished& testRunHookFinished) const
    {
        return FindOne(testRunHookStartedById, testRunHookFinished.testRunHookStartedId);
    }

    const messages::TestRunHookFinished* Query::FindTestRunHookFinishedBy(const messages::TestRunHookStarted& testRunHookStarted) const
    {
        return FindOne(testRunHookFinishedByTestRunHookStartedId, testRunHookStarted.id);
    }

    std::optional<messages::Duration> Query::FindTestRunDuration() const
    {
        if (testRunStarted != nullptr && testRunFinished != nullptr)
        {
            return testRunFinished->timestamp - testRunStarted->timestamp;
        }

        return std::nullopt;
    }

    const messages::TestRunFinished* Query::FindTestRunFinished() const
    {
        return testRunFinished;
    }

    const messages::TestRunStarted* Query::FindTestRunStarted() const
    {
        return testRunStarted;
    }

    const messages::TestStep* Query::FindTestStepBy(const messages::TestStepStarted& element) const
    {
        return FindOne(testStepById, element.testStepId);
    }

    const messages::TestStep* Query::FindTestStepBy(const messages::TestStepFinished& element) const
    {
        return FindOne(testStepById, element.testStepId);
    }

    ElementsView<messages::TestStepStarted> Query::FindTestStepsStartedBy(const messages::TestCaseStarted& testCaseStarted) const
    {
        return FindMany(testStepStartedByTestCaseStartedId, testCaseStarted.id) | views::Dereference();
    }

    ElementsView<messages::TestStepStarted> Query::FindTestStepsStartedBy(const messages::TestCaseFinished& testCaseFinished) const
    {
        return FindMany(testStepStartedByTestCaseStartedId, testCaseFinished.testCaseStartedId) | views::Dereference();
    }

    ElementsView<messages::TestStepFinished> Query::FindTestStepsFinishedBy(const messages::TestCaseStarted& element) const
    {
        return FindMany(testStepFinishedByTestCaseStartedId, element.id) | views::Dereference();
    }

    ElementsView<messages::TestStepFinished> Query::FindTestStepsFinishedBy(const messages::TestCaseFinished& element) const
    {
        return FindMany(testStepFinishedByTestCaseStartedId, element.testCaseStartedId) | views::Dereference();
    }

    std::vector<TestStepFinishedAndTestStep> Query::FindTestStepFinishedAndTestStepBy(const messages::TestCaseStarted& testCaseStarted) const
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

    std::optional<LineageAndPickle> Query::FindLineageBy(const messages::Pickle& element) const
    {
        const auto* lineage = FindOne(lineageById, element.astNodeIds.back());

        if (lineage != nullptr)
        {
            return LineageAndPickle{ lineage, &element };
        }

        return std::nullopt;
    }

    std::optional<LineageAndPickle> Query::FindLineageBy(const messages::TestCaseStarted& element) const
    {
        const auto* pickle = FindPickleBy(element);

        if (pickle != nullptr)
        {
            return FindLineageBy(*pickle);
        }

        return std::nullopt;
    }

    std::optional<LineageAndPickle> Query::FindLineageBy(const messages::TestCaseFinished& element) const
    {
        const auto* pickle = FindPickleBy(element);

        if (pickle != nullptr)
        {
            return FindLineageBy(*pickle);
        }

        return std::nullopt;
    }

    std::vector<const messages::TestCaseStarted*> Query::AllTestCaseStarted() const
    {
        std::vector<const messages::TestCaseStarted*> result;

        for (const auto& testCaseStarted : FindAllTestCaseStarted())
        {
            result.push_back(&testCaseStarted);
        }

        return result;
    }

    std::vector<const messages::TestCaseFinished*> Query::AllTestCaseFinished() const
    {
        std::vector<const messages::TestCaseFinished*> result;

        for (const auto& testCaseFinished : FindAllTestCaseFinished())
        {
            result.push_back(&testCaseFinished);
        }

        return result;
    }

    void Query::UpdateGherkinDocument(const messages::GherkinDocument& gherkinDocument)
    {
        if (gherkinDocument.feature)
        {
            UpdateFeature(*gherkinDocument.feature, Lineage{ &gherkinDocument });
        }
    }

    void Query::UpdateFeature(const messages::Feature& feature, Lineage lineage)
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

    void Query::UpdateRule(const messages::Rule& rule, Lineage lineage)
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

    void Query::UpdateScenario(const messages::Scenario& scenario, const Lineage& lineage)
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

    void Query::UpdateSteps(const std::vector<messages::Step>& steps)
    {
        for (const auto& step : steps)
        {
            stepById[step.id] = std::addressof(step);
        }
    }

    void Query::UpdatePickle(const messages::Pickle& pickle)
    {
        pickleById[pickle.id] = &pickle;
        for (const auto& pickleStep : pickle.steps)
        {
            pickleStepById[pickleStep.id] = std::addressof(pickleStep);
        }
    }

    void Query::UpdateTestRunHookStarted(const messages::TestRunHookStarted& testRunHookStarted)
    {
        testRunHookStartedById[testRunHookStarted.id] = &testRunHookStarted;
    }

    void Query::UpdateTestRunHookFinished(const messages::TestRunHookFinished& testRunHookFinished)
    {
        testRunHookFinishedByTestRunHookStartedId[testRunHookFinished.testRunHookStartedId] = &testRunHookFinished;
    }

    void Query::UpdateTestCase(const messages::TestCase& testCase)
    {
        for (const auto& testStep : testCase.testSteps)
        {
            testStepById[testStep.id] = std::addressof(testStep);
        }
        testCaseById[testCase.id] = &testCase;
    }

    void Query::UpdateTestCaseStarted(const messages::TestCaseStarted& testCaseStarted)
    {
        testCaseStartedById[testCaseStarted.id] = &testCaseStarted;
    }

    void Query::UpdateAttachment(const messages::Attachment& attachment)
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

    void Query::UpdateTestStepFinished(const messages::TestStepFinished& testStepFinished)
    {
        testStepFinishedByTestCaseStartedId[testStepFinished.testCaseStartedId].push_back(&testStepFinished);
    }

    void Query::UpdateTestCaseFinished(const messages::TestCaseFinished& testCaseFinished)
    {
        testCaseFinishedByTestCaseStartedId[testCaseFinished.testCaseStartedId] = &testCaseFinished;
    }
}
