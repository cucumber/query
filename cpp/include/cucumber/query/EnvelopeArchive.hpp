#ifndef CUCUMBER_QUERY_ENVELOPE_ARCHIVE_HPP
#define CUCUMBER_QUERY_ENVELOPE_ARCHIVE_HPP

#include "cucumber/messages/Envelope.hpp"
#include <deque>

namespace cucumber::query
{
    struct EnvelopeArchive
    {
        auto Store(messages::Envelope&& envelope) -> const messages::Envelope&;

    private:
        std::deque<messages::Envelope> envelopes;
    };
}

#endif
