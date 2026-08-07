#ifndef CUCUMBER_QUERY_NAMING_STRATEGY_HPP
#define CUCUMBER_QUERY_NAMING_STRATEGY_HPP

#include "cucumber/messages/Pickle.hpp"
#include "cucumber/query/Lineage.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace cucumber::query
{
    struct NamingStrategy
    {
        NamingStrategy() = default;

        NamingStrategy(const NamingStrategy&) = default;
        NamingStrategy(NamingStrategy&&) = default;
        auto operator=(const NamingStrategy&) -> NamingStrategy& = default;
        auto operator=(NamingStrategy&&) -> NamingStrategy& = default;

        virtual ~NamingStrategy() = default;

        [[nodiscard]] virtual auto Reduce(const Lineage& lineage, const messages::Pickle& pickle) const -> std::string = 0;
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

        [[nodiscard]] auto Reduce(const Lineage& lineage, const messages::Pickle& pickle) const -> std::string override;

    private:
        NamingStrategyLength length;
        NamingStrategyFeatureName featureName;
        NamingStrategyExampleName exampleName;
    };

    auto CreateNamingStrategy(NamingStrategyLength length, NamingStrategyFeatureName featureName = NamingStrategyFeatureName::include,
        NamingStrategyExampleName exampleName = NamingStrategyExampleName::numberAndPickleIfParameterized) -> std::unique_ptr<const NamingStrategy>;
}

#endif
