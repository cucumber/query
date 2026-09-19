#ifndef CUCUMBER_QUERY_NDJSON_ENVELOPE_READER_HPP
#define CUCUMBER_QUERY_NDJSON_ENVELOPE_READER_HPP

#include "cucumber/messages/Envelope.hpp"
#include "cucumber/query/EnvelopeArchive.hpp"
#include <istream>
#include <optional>
#include <utility>

namespace cucumber::query
{
    namespace detail
    {
        std::optional<messages::Envelope> TryReadNextEnvelope(std::istream& input);
    }

    template<typename Observer>
    void LoadNdjson(EnvelopeArchive& archive, std::istream& input, Observer&& observer)
    {
        while (auto envelope = detail::TryReadNextEnvelope(input))
        {
            std::forward<Observer>(observer)(archive.Store(std::move(*envelope)));
        }
    }
}

#endif
