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
#include <memory>
#include <optional>

namespace cucumber::query
{
    struct Lineage
    {
        std::shared_ptr<const messages::GherkinDocument> gherkinDocument;
        std::shared_ptr<const messages::Feature> feature;
        std::shared_ptr<const messages::Background> background;
        std::shared_ptr<const messages::Rule> rule;
        std::shared_ptr<const messages::Background> ruleBackground;
        std::shared_ptr<const messages::Scenario> scenario;
        std::shared_ptr<const messages::Examples> examples;
        std::optional<std::size_t> examplesIndex;
        std::shared_ptr<const messages::TableRow> example;
        std::optional<std::size_t> exampleIndex;

        Lineage operator+(const Lineage& other) const;
    };
}

#endif
