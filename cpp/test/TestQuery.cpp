#include "cucumber/messages/All.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/Query.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

TEST(first, second)
{}

namespace cucumber::query
{
    namespace
    {
        std::set<std::filesystem::path, std::less<>> CollectFiles(const std::filesystem::path& folder)
        {
            std::set<std::filesystem::path, std::less<>> foundFiles;

            for (const auto& entry : std::filesystem::directory_iterator{ folder })
            {
                foundFiles.emplace(entry.path());
            }

            return foundFiles;
        }

        struct TestData
        {
            std::list<std::filesystem::path> ndjson;
            std::list<std::filesystem::path> json;
            std::set<std::filesystem::path, std::less<>> txt;
        };

        struct TestSet
        {
            std::filesystem::path ndjson;
            std::set<std::filesystem::path, std::less<>> json;
        };

        std::int32_t ReversePickleComparator(const std::shared_ptr<const messages::Pickle>& lhs, const std::shared_ptr<const messages::Pickle>& rhs)
        {
            if (lhs->uri != rhs->uri)
            {
                return static_cast<std::int32_t>(lhs->uri.compare(rhs->uri));
            }
            if (lhs->location.has_value() && rhs->location.has_value() && lhs->location.value()->line != rhs->location.value()->line)
            {
                return static_cast<std::int32_t>(rhs->location.value()->line) - static_cast<std::int32_t>(lhs->location.value()->line);
            }
            return static_cast<std::int32_t>(rhs->location.value()->column.value()) - static_cast<std::int32_t>(lhs->location.value()->column.value());
        };

        std::optional<std::shared_ptr<const messages::Pickle>> FindPickleBy(const query::Query& query,
            std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>, std::shared_ptr<const messages::TestStepStarted>> element)
        {
            return query.FindPickleBy(std::move(element));
        };

        template<auto F>
        nlohmann::json GetSizeOf(const query::Query& query)
        {
            const auto actual = (query.*F)();
            return actual.size();
        }

        nlohmann::json CountMostSevereTestStepResultStatus(const query::Query& query)
        {
            nlohmann::json actual;

            for (const auto& [status, count] : query.CountMostSevereTestStepResultStatus())
            {
                actual[to_string(status)] = count;
            }

            return actual;
        }

        nlohmann::json CountTestCasesStarted(const query::Query& query)
        {
            const auto actual = query.CountTestCasesStarted();
            return actual;
        }

        nlohmann::json FindAllTestCaseFinishedOrderBy(const query::Query& query)
        {
            const auto allResults = query.FindAllTestCaseFinishedOrderBy(FindPickleBy, ReversePickleComparator);
            nlohmann::json actual;
            for (const auto& testCaseFinished : allResults)
            {
                actual.push_back(testCaseFinished->testCaseStartedId);
            }

            return actual;
        }

        nlohmann::json FindAllTestCaseStartedOrderBy(const query::Query& query)
        {
            const auto allResults = query.FindAllTestCaseStartedOrderBy(&FindPickleBy, ReversePickleComparator);
            nlohmann::json actual;
            for (const auto& testCaseStarted : allResults)
            {
                actual.push_back(testCaseStarted->id);
            }

            return actual;
        }

        nlohmann::json FindAllUndefinedParameterTypes(query::Query const& query)
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& undefinedParameterType : query.FindAllUndefinedParameterTypes())
            {
                actual.push_back({ undefinedParameterType->name, undefinedParameterType->expression });
            }

            return actual;
        }

        nlohmann::json FindAttachmentsBy(const query::Query& query)
        {
            nlohmann::json actual = { { "testStepFinished", nlohmann::json::array() }, { "testRunHookFinished", nlohmann::json::array() } };

            const auto& allTestCaseStarted = query.FindAllTestCaseStarted();
            for (const auto& testCaseStarted : allTestCaseStarted)
            {
                const auto& allFinishedSteps = query.FindTestStepsFinishedBy(testCaseStarted);
                for (const auto& testStepFinished : allFinishedSteps)
                {
                    const auto& attachments = query.FindAttachmentsBy(testStepFinished);

                    for (const auto& attachment : attachments)
                    {
                        actual["testStepFinished"].push_back({ attachment->testStepId, attachment->testCaseStartedId, attachment->mediaType, attachment->contentEncoding });
                    }
                }
            }

            const auto& allTestRunHookFinished = query.FindAllTestRunHookFinished();
            for (const auto& testRunHookFinished : allTestRunHookFinished)
            {
                const auto& attachments = query.FindAttachmentsBy(testRunHookFinished);

                for (const auto& attachment : attachments)
                {
                    actual["testRunHookFinished"].push_back({ attachment->testRunHookStartedId, attachment->mediaType, attachment->contentEncoding });
                }
            }

            if (actual["testStepFinished"].is_array())
            {
                std::sort(actual["testStepFinished"].begin(), actual["testStepFinished"].end(),
                    [](const nlohmann::json& lhs, const nlohmann::json& rhs)
                    {
                        return std::stoi(lhs[0].get<std::string>()) < std::stoi(rhs[0].get<std::string>());
                    });
            }

            if (actual["testRunHookFinished"].is_array())
            {
                std::sort(actual["testRunHookFinished"].begin(), actual["testRunHookFinished"].end(),
                    [](const nlohmann::json& lhs, const nlohmann::json& rhs)
                    {
                        return std::stoi(lhs[0].get<std::string>()) < std::stoi(rhs[0].get<std::string>());
                    });
            }

            return actual;
        }

        const std::unordered_map<std::string_view, nlohmann::json (*)(const query::Query&)> functionMap{
            { "countMostSevereTestStepResultStatus", CountMostSevereTestStepResultStatus },
            { "countTestCasesStarted", CountTestCasesStarted },
            { "findAllPickles", GetSizeOf<&query::Query::FindAllPickles> },
            { "findAllPickleSteps", GetSizeOf<&query::Query::FindAllPickleSteps> },
            { "findAllStepDefinitions", GetSizeOf<&query::Query::FindAllStepDefinitions> },
            { "findAllTestCaseFinished", GetSizeOf<&query::Query::FindAllTestCaseFinished> },
            { "findAllTestCaseFinishedOrderBy", FindAllTestCaseFinishedOrderBy },
            { "findAllTestCases", GetSizeOf<&query::Query::FindAllTestCases> },
            { "findAllTestCaseStarted", GetSizeOf<&query::Query::FindAllTestCaseStarted> },
            { "findAllTestCaseStartedOrderBy", FindAllTestCaseStartedOrderBy },
            { "findAllTestRunHookFinished", GetSizeOf<&query::Query::FindAllTestRunHookFinished> },
            { "findAllTestRunHookStarted", GetSizeOf<&query::Query::FindAllTestRunHookStarted> },
            { "findAllTestStepFinished", GetSizeOf<&query::Query::FindAllTestStepFinished> },
            { "findAllTestSteps", GetSizeOf<&query::Query::FindAllTestSteps> },
            { "findAllTestStepStarted", GetSizeOf<&query::Query::FindAllTestStepStarted> },
            { "findAllUndefinedParameterTypes", FindAllUndefinedParameterTypes },
            { "findAttachmentsBy", FindAttachmentsBy },
        };

        struct AcceptanceTest : testing::Test
        {
            AcceptanceTest(std::filesystem::path input, std::filesystem::path expected)
                : input{ std::move(input) }
                , expected{ std::move(expected) }
            {}

        private:
            void TestBody() override
            {
                const auto* testInfo = testing::UnitTest::GetInstance()->current_test_info();
                const auto* testCaseName = testInfo->name();

                if (functionMap.find(testCaseName) == functionMap.end())
                {
                    GTEST_SKIP() << "No function registered for test case: " << testCaseName;
                }

                // load Query with messages
                {
                    std::ifstream ifstream{ input };
                    std::string line;
                    while (std::getline(ifstream, line))
                    {
                        messages::Envelope envelope;
                        envelope.from_json(nlohmann::json::parse(line));

                        query.Update(envelope);
                    }
                }

                {
                    std::ifstream ifstream{ expected };
                    EXPECT_THAT(functionMap.at(testCaseName)(query), testing::Eq(nlohmann::json::parse(ifstream)));
                }
            }

            query::Query query;

            std::filesystem::path input;
            std::filesystem::path expected;
        };

        TestData GetTestFiles()
        {
            auto foundFiles = CollectFiles(TESTDATA_SRC);

            TestData testData{};

            while (!foundFiles.empty())
            {
                const auto begin = foundFiles.begin();
                if (begin->extension() == ".ndjson")
                {
                    testData.ndjson.emplace_back(std::move(foundFiles.extract(begin).value()));
                }
                else if (begin->extension() == ".json")
                {
                    testData.json.emplace_back(std::move(foundFiles.extract(begin).value()));
                }
                else if (begin->extension() == ".txt")
                {
                    testData.txt.emplace(std::move(foundFiles.extract(begin).value()));
                }
                else
                {
                    throw "unknown extension:" + begin->extension().string();
                }
            }

            return testData;
        }

        std::string FilterName(std::string str)
        {
            for (auto iter = str.find('-'); iter != std::string::npos; iter = str.find('-'))
            {
                str.replace(iter, 1, "_");
            }

            return str;
        }
    }
}

GTEST_API_ int main(int argc, char** argv)
{
    using namespace cucumber::query;
    auto testData = GetTestFiles();

    std::vector<TestSet> testSets{};

    for (auto ndjsonIter = testData.ndjson.begin(); ndjsonIter != testData.ndjson.end();)
    {
        TestSet testSet{};
        const auto& testSetName = ndjsonIter->stem().string();

        for (auto iter = testData.json.begin(); iter != testData.json.end();)
        {
            if (iter->filename().string().find(testSetName) == 0)
            {
                testSet.json.emplace(std::move(*iter));
                testData.json.erase(std::exchange(iter, std::next(iter)));
            }
            else
            {
                iter = std::next(iter);
            }
        }

        if (!testSet.json.empty())
        {
            testSet.ndjson = std::move(*ndjsonIter);
            testSets.push_back(std::move(testSet));

            testData.ndjson.erase(std::exchange(ndjsonIter, std::next(ndjsonIter)));
        }
        else
        {
            ndjsonIter = std::next(ndjsonIter);
        }
    }

    for (const auto& suite : testSets)
    {
        for (const auto& test : suite.json)
        {
            const auto suiteName = suite.ndjson.stem().string();
            const auto testNameResult = test.stem().string().substr(suiteName.size() + 1);
            const auto testName = testNameResult.substr(0, testNameResult.size() - std::strlen(".results"));

            ::testing::RegisterTest(FilterName(suiteName).c_str(), FilterName(testName).c_str(), nullptr, nullptr, test.c_str(), 1,
                [ndjson = suite.ndjson, json = test]() -> AcceptanceTest*
                {
                    return new AcceptanceTest(ndjson, json); // NOLINT(cppcoreguidelines-owning-memory)
                });
        }
    }

    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
