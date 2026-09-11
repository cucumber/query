#ifndef CUCUMBER_QUERY_LINEAGE_HPP
#define CUCUMBER_QUERY_LINEAGE_HPP

#include "cucumber/messages/Background.hpp"
#include "cucumber/messages/Examples.hpp"
#include "cucumber/messages/Feature.hpp"
#include "cucumber/messages/GherkinDocument.hpp"
#include "cucumber/messages/Rule.hpp"
#include "cucumber/messages/Scenario.hpp"
#include "cucumber/messages/TableRow.hpp"
#include <cstddef>
#include <optional>

namespace cucumber::query
{
    struct Lineage
    {
        const messages::GherkinDocument* gherkinDocument{ nullptr };
        const messages::Feature* feature{ nullptr };
        const messages::Background* background{ nullptr };
        const messages::Rule* rule{ nullptr };
        const messages::Background* ruleBackground{ nullptr };
        const messages::Scenario* scenario{ nullptr };
        const messages::Examples* examples{ nullptr };
        std::optional<std::size_t> examplesIndex;
        const messages::TableRow* example{ nullptr };
        std::optional<std::size_t> exampleIndex;

        auto operator+(const Lineage& other) const -> Lineage;
    };
}

#endif
