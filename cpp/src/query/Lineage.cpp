#include "cucumber/query/Lineage.hpp"

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
        combined.examplesIndex = other.examplesIndex ? other.examplesIndex : examplesIndex;
        combined.example = other.example ? other.example : example;
        combined.exampleIndex = other.exampleIndex ? other.exampleIndex : exampleIndex;

        return combined;
    }
}
