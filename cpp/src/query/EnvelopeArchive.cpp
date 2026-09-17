#include "cucumber/query/EnvelopeArchive.hpp"
#include "cucumber/messages/Envelope.hpp"
#include <utility>

namespace cucumber::query
{
    const messages::Envelope& EnvelopeArchive::Store(messages::Envelope&& envelope)
    {
        return envelopes.emplace_back(std::move(envelope));
    }
}
