#include "cucumber/query/NamingStrategy.hpp"
#include "cucumber/query/Lineage.hpp"
#include <cstddef>
#include <cucumber/messages/Pickle.hpp>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace cucumber::query
{
    BuiltinNamingStrategy::BuiltinNamingStrategy(NamingStrategyLength length, NamingStrategyFeatureName featureName, NamingStrategyExampleName exampleName)
        : length{ length }
        , featureName{ featureName }
        , exampleName{ exampleName }
    {}

    auto BuiltinNamingStrategy::Reduce(const Lineage& lineage, const messages::Pickle& pickle) const -> std::string
    {
        static constexpr std::size_t namingStrategyPartsCount = 8;

        std::vector<std::string> parts(namingStrategyPartsCount);

        if (lineage.feature && !lineage.feature->name.empty() && featureName == NamingStrategyFeatureName::include)
        {
            parts.emplace_back(lineage.feature->name);
        }

        if (lineage.rule && !lineage.rule->name.empty())
        {
            parts.emplace_back(lineage.rule->name);
        }

        if (lineage.scenario && !lineage.scenario->name.empty())
        {
            parts.emplace_back(lineage.scenario->name);
        }
        else
        {
            parts.emplace_back(pickle.name);
        }

        if (lineage.examples && !lineage.examples->name.empty())
        {
            parts.emplace_back(lineage.examples->name);
        }

        if (lineage.example)
        {
            const auto exampleNumber = "#" + std::to_string(lineage.examplesIndex.value_or(0) + 1) + "." + std::to_string(lineage.exampleIndex.value_or(0) + 1);

            switch (exampleName)
            {
                case NamingStrategyExampleName::number:
                    parts.emplace_back(exampleNumber);
                    break;
                case NamingStrategyExampleName::pickle:
                    parts.emplace_back(pickle.name);
                    break;
                case NamingStrategyExampleName::numberAndPickleIfParameterized:
                    if (lineage.scenario && lineage.scenario->name != pickle.name)
                    {
                        parts.emplace_back(exampleNumber + ": " + pickle.name);
                    }
                    else
                    {
                        parts.emplace_back(exampleNumber);
                    }
                    break;
            }
        }

        if (this->length == NamingStrategyLength::shortName)
        {
            return parts.back();
        }

        return std::accumulate(parts.begin(), parts.end(), std::string{},
            [](const std::string& cumulative, const std::string& element)
            {
                if (element.empty())
                {
                    return cumulative;
                }

                if (cumulative.empty())
                {
                    return element;
                }

                return cumulative + " - " + element;
            });
    }

    auto CreateNamingStrategy(NamingStrategyLength length, NamingStrategyFeatureName featureName, NamingStrategyExampleName exampleName) -> std::unique_ptr<const NamingStrategy>
    {
        return std::make_unique<const BuiltinNamingStrategy>(length, featureName, exampleName);
    }
}
