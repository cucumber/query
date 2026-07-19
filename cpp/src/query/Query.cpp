#include "cucumber/query/Query.hpp"
#include "cucumber/messages/All.hpp"
#include <cstddef>
#include <cucumber/messages/TestStepResultStatus.hpp>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cucumber::query
{

    Lineage Lineage::operator+(const Lineage& other) const
    {
        Lineage combined;
        combined.gherkinDocument = other.gherkinDocument ? other.gherkinDocument : gherkinDocument;
        combined.feature = other.feature ? other.feature : feature;
        combined.background = other.background ? other.background : background;
        combined.rule = other.rule ? other.rule : rule;
        combined.ruleBackground = other.ruleBackground ? other.ruleBackground : ruleBackground;
        combined.scenario = other.scenario ? other.scenario : scenario;
        combined.examples = other.examples ? other.examples : examples;
        combined.examplesIndex = (other.examplesIndex != 0) ? other.examplesIndex : examplesIndex;
        combined.example = other.example ? other.example : example;
        combined.exampleIndex = (other.exampleIndex != 0) ? other.exampleIndex : exampleIndex;
        return combined;
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
        // if (envelope.hook)
        // {
        //     this.hookById.set(envelope.hook.id, envelope.hook)
        // }
        // if (envelope.stepDefinition)
        // {
        //     this.stepDefinitionById.set(envelope.stepDefinition.id, envelope.stepDefinition)
        // }
        // if (envelope.testRunStarted)
        // {
        //     this.testRunStarted = envelope.testRunStarted
        // }
        // if (envelope.testRunHookStarted)
        // {
        //     this.updateTestRunHookStarted(envelope.testRunHookStarted)
        // }
        // if (envelope.testRunHookFinished)
        // {
        //     this.updateTestRunHookFinished(envelope.testRunHookFinished)
        // }
        // if (envelope.testCase)
        // {
        //     this.updateTestCase(envelope.testCase)
        // }
        if (envelope.testCaseStarted.has_value())
        {
            UpdateTestCaseStarted(envelope.testCaseStarted.value());
        }
        // if (envelope.testStepStarted)
        // {
        //     this.updateTestStepStarted(envelope.testStepStarted)
        // }
        // if (envelope.attachment)
        // {
        //     this.updateAttachment(envelope.attachment)
        // }
        // if (envelope.testStepFinished)
        // {
        //     this.updateTestStepFinished(envelope.testStepFinished)
        // }
        // if (envelope.testCaseFinished)
        // {
        //     this.updateTestCaseFinished(envelope.testCaseFinished)
        // }
        // if (envelope.testRunFinished)
        // {
        //     this.testRunFinished = envelope.testRunFinished
        // }
        // if (envelope.suggestion)
        // {
        //     this.updateSuggestion(envelope.suggestion)
        // }
        // if (envelope.undefinedParameterType)
        // {
        //     this.updateUndefinedParameterType(envelope.undefinedParameterType)
        // }
    }

    std::unordered_map<messages::TestStepResultStatus, std::size_t> Query::CountMostSevereTestStepResultStatus() const
    {
        return {};
    }

    std::size_t Query::CountTestCasesStarted() const
    {
        return FindAllTestCaseStarted().size();
    }

    std::vector<std::shared_ptr<const messages::Pickle>> Query::FindAllPickles() const
    {
        std::vector<std::shared_ptr<const messages::Pickle>> result;
        result.reserve(pickleById.size());

        for (const auto& [pickleId, pickle] : pickleById)
        {
            result.push_back(pickle);
        }

        return result;
    }

    std::vector<std::shared_ptr<const messages::PickleStep>> Query::FindAllPickleSteps() const
    {
        return {};
    }

    std::vector<std::shared_ptr<const messages::StepDefinition>> Query::FindAllStepDefinitions() const
    {
        return {};
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

        return result;
    }

    std::vector<std::shared_ptr<const messages::TestCaseFinished>> Query::FindAllTestCaseFinished() const
    {
        std::vector<std::shared_ptr<const messages::TestCaseFinished>> result;

        for (const auto& [testCaseStartedId, testCaseFinished] : testCaseFinishedByTestCaseStartedId)
        {
            if (testCaseFinished->willBeRetried)
            {
                result.push_back(testCaseFinished);
            }
        }

        return result;
    }

    // [[nodiscard]] std::size_t Query::CountTestCasesStarted() const
    // {
    //     return testCaseStartedById.size();
    // }

    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
    /////////////////////////////

    void Query::UpdateGherkinDocument(const std::shared_ptr<const messages::GherkinDocument>& gherkinDocument)
    {
        if (gherkinDocument->feature.has_value())
        {
            UpdateFeature(gherkinDocument->feature.value(), std::make_unique<Lineage>(Lineage{ gherkinDocument }));
        }
    }

    void Query::UpdateFeature(const std::shared_ptr<const messages::Feature>& feature, std::unique_ptr<Lineage> lineage)
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
                UpdateScenario(featureChild->scenario.value(), std::make_unique<Lineage>(*lineage + Lineage{ {}, feature }));
            }

            if (featureChild->rule.has_value())
            {
                UpdateRule(featureChild->rule.value(), std::make_unique<Lineage>(*lineage + Lineage{ {}, feature }));
            }
        }
    }

    void Query::UpdateRule(const std::shared_ptr<const messages::Rule>& rule, std::unique_ptr<Lineage> lineage)
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
                UpdateScenario(ruleChild->scenario.value(), std::make_unique<Lineage>(*lineage + Lineage{ {}, {}, {}, rule }));
            }
        }
    }

    void Query::UpdateScenario(const std::shared_ptr<const messages::Scenario>& scenario, std::unique_ptr<Lineage> lineage)
    {
        lineageById[scenario->id] = std::make_unique<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario });

        std::size_t examplesIndex = 0;
        for (const auto& examples : scenario->examples)
        {
            lineageById[examples->id] = std::make_unique<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario, examples, examplesIndex });

            std::size_t exampleIndex = 0;
            for (const auto& example : examples->tableBody)
            {
                lineageById[example->id] = std::make_unique<Lineage>(*lineage + Lineage{ {}, {}, {}, {}, {}, scenario, examples, examplesIndex, example, exampleIndex });
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
        pickleById[pickle->id] = std::move(pickle);
        // for (const pickleStep of pickle.steps) {
        //   this.pickleStepById.set(pickleStep.id, pickleStep)
    }

    /////////////////////////////
    /////////////////////////////
    /////////////////////////////

    void Query::UpdateTestCaseStarted(std::shared_ptr<const messages::TestCaseStarted> testCaseStarted)
    {
        testCaseStartedById[testCaseStarted->id] = std::move(testCaseStarted);
    }

    /////////////////////////////
    /////////////////////////////
    /////////////////////////////
}
