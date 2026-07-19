#include "cucumber/query/Query.hpp"
#include "cucumber/messages/Envelope.hpp"

namespace cucumber::query
{
    void Query::Update(const cucumber::messages::Envelope& envelope)
    {}

    [[nodiscard]] std::size_t Query::CountTestCasesStarted() const
    {
        return testCaseStartedById.size();
    }
}
