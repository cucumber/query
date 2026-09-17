#ifndef CUCUMBER_QUERY_ENVELOPE_ARCHIVE_HPP
#define CUCUMBER_QUERY_ENVELOPE_ARCHIVE_HPP

#include "cucumber/messages/Envelope.hpp"
#include <deque>

namespace cucumber::query
{
    struct EnvelopeArchive
    {
        const messages::Envelope& Store(messages::Envelope&& envelope);

    private:
        std::deque<messages::Envelope> envelopes;
    };
}

#endif
