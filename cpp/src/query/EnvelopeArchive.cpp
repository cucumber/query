#include "cucumber/query/EnvelopeArchive.hpp"
#include "cucumber/messages/Envelope.hpp"
#include <utility>

namespace cucumber::query
{
    auto EnvelopeArchive::Store(messages::Envelope&& envelope) -> const messages::Envelope&
    {
        return envelopes.emplace_back(std::move(envelope));
    }
}
