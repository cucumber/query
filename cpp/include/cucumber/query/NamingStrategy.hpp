#ifndef CUCUMBER_QUERY_NAMING_STRATEGY_HPP
#define CUCUMBER_QUERY_NAMING_STRATEGY_HPP

#include "cucumber/messages/Pickle.hpp"
#include "cucumber/query/Lineage.hpp"
#include <cstdint>
#include <string>

namespace cucumber::query
{
    struct NamingStrategy
    {
        NamingStrategy() = default;

        NamingStrategy(const NamingStrategy&) = default;
        NamingStrategy(NamingStrategy&&) = default;
        NamingStrategy& operator=(const NamingStrategy&) = default;
        NamingStrategy& operator=(NamingStrategy&&) = default;

        virtual ~NamingStrategy() = default;

        [[nodiscard]] virtual std::string Reduce(const Lineage& lineage, const messages::Pickle& pickle) const = 0;
    };

    enum class NamingStrategyLength : std::uint8_t
    {
        longName,
        shortName,
    };

    enum class NamingStrategyFeatureName : std::uint8_t
    {
        include,
        exclude,
    };

    enum class NamingStrategyExampleName : std::uint8_t
    {
        number,
        pickle,
        numberAndPickleIfParameterized,
    };

    struct BuiltinNamingStrategy : NamingStrategy
    {
        BuiltinNamingStrategy(NamingStrategyLength length, NamingStrategyFeatureName featureName, NamingStrategyExampleName exampleName);

        [[nodiscard]] std::string Reduce(const Lineage& lineage, const messages::Pickle& pickle) const override;

    private:
        NamingStrategyLength length;
        NamingStrategyFeatureName featureName;
        NamingStrategyExampleName exampleName;
    };

    std::unique_ptr<const NamingStrategy> CreateNamingStrategy(NamingStrategyLength length, NamingStrategyFeatureName featureName = NamingStrategyFeatureName::include,
        NamingStrategyExampleName exampleName = NamingStrategyExampleName::numberAndPickleIfParameterized);
}

#endif
