#include "cucumber/query/Lineage.hpp"

namespace cucumber::query
{
    Lineage Lineage::operator+(const Lineage& other) const
    {
        Lineage combined;

        combined.gherkinDocument = (other.gherkinDocument != nullptr) ? other.gherkinDocument : gherkinDocument;
        combined.feature = (other.feature != nullptr) ? other.feature : feature;
        combined.background = (other.background != nullptr) ? other.background : background;
        combined.rule = (other.rule != nullptr) ? other.rule : rule;
        combined.ruleBackground = (other.ruleBackground != nullptr) ? other.ruleBackground : ruleBackground;
        combined.scenario = (other.scenario != nullptr) ? other.scenario : scenario;
        combined.examples = (other.examples != nullptr) ? other.examples : examples;
        combined.examplesIndex = other.examplesIndex ? other.examplesIndex : examplesIndex;
        combined.example = (other.example != nullptr) ? other.example : example;
        combined.exampleIndex = other.exampleIndex ? other.exampleIndex : exampleIndex;

        return combined;
    }
}
