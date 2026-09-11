#include "cucumber/query/NdjsonEnvelopeReader.hpp"
#include "cucumber/messages/Envelope.hpp"
#include <istream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

namespace cucumber::query::detail
{
    auto TryReadNextEnvelope(std::istream& input) -> std::optional<messages::Envelope>
    {
        std::string line;
        if (!std::getline(input, line))
        {
            return std::nullopt;
        }

        messages::Envelope envelope{};
        envelope.from_json(nlohmann::json::parse(line));
        return envelope;
    }
}
