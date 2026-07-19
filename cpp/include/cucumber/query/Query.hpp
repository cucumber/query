#ifndef CUCUMBER_QUERY_QUERY_HPP
#define CUCUMBER_QUERY_QUERY_HPP

#include <cstddef>
#include <cucumber/messages/Envelope.hpp>
#include <cucumber/messages/TestCaseStarted.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace cucumber::query
{
    class Query
    {
    public:
        void Update(const cucumber::messages::Envelope& envelope);

        [[nodiscard]] std::size_t CountTestCasesStarted() const;

    private:
        std::unordered_map<std::string, std::shared_ptr<messages::TestCaseStarted>, std::less<>> testCaseStartedById;
    };
}

#endif
