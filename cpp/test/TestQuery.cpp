#include "cucumber/messages/Envelope.hpp"
#include "cucumber/messages/Pickle.hpp"
#include "cucumber/messages/TestCaseFinished.hpp"
#include "cucumber/messages/TestCaseStarted.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/Query.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

TEST(first, second)
{}

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

    std::int32_t ReversePickleComparator(const std::shared_ptr<const cucumber::messages::Pickle>& lhs, const std::shared_ptr<const cucumber::messages::Pickle>& rhs)
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

    void CountMostSevereTestStepResultStatus(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto allResults = query.CountMostSevereTestStepResultStatus();
        nlohmann::json actual;
        for (const auto& [status, count] : allResults)
        {
            actual[to_string(status)] = count;
        }

        EXPECT_THAT(actual, testing::Eq(expected));
    }

    void CountTestCasesStarted(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.CountTestCasesStarted();
        EXPECT_THAT(actual, testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllPickles(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllPickles();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllPickleSteps(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllPickleSteps();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllStepDefinitions(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllStepDefinitions();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllTestCaseFinished(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllTestCaseFinished();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllTestCaseFinishedOrderBy(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        auto findOrderBy = [](const cucumber::query::Query& query, std::shared_ptr<const cucumber::messages::TestCaseFinished> testCaseFinished)
        {
            return query.FindPickleBy(testCaseFinished);
        };

        const auto allResults = query.FindAllTestCaseFinishedOrderBy(findOrderBy, ReversePickleComparator);
        nlohmann::json actual;
        for (const auto& testCaseFinished : allResults)
        {
            actual.push_back(testCaseFinished->testCaseStartedId);
        }

        EXPECT_THAT(actual, testing::Eq(expected));
    }

    void FindAllTestCases(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllTestCases();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllTestCaseStarted(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllTestCaseStarted();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    void FindAllTestCaseStartedOrderBy(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        auto findOrderBy = [](const cucumber::query::Query& query, std::shared_ptr<const cucumber::messages::TestCaseStarted> testCaseStarted)
        {
            return query.FindPickleBy(testCaseStarted);
        };

        const auto allResults = query.FindAllTestCaseStartedOrderBy(findOrderBy, ReversePickleComparator);
        nlohmann::json actual;
        for (const auto& testCaseStarted : allResults)
        {
            actual.push_back(testCaseStarted->id);
        }

        EXPECT_THAT(actual, testing::Eq(expected));
    }

    void FindAllTestRunHookFinished(const cucumber::query::Query& query, const nlohmann::json& expected)
    {
        const auto actual = query.FindAllTestRunHookFinished();
        EXPECT_THAT(actual.size(), testing::Eq(expected.get<std::size_t>()));
    }

    const std::unordered_map<std::string_view, void (*)(const cucumber::query::Query&, const nlohmann::json&)> functionMap{
        { "countMostSevereTestStepResultStatus", &CountMostSevereTestStepResultStatus },
        { "countTestCasesStarted", &CountTestCasesStarted },
        { "findAllPickles", &FindAllPickles },
        { "findAllPickleSteps", &FindAllPickleSteps },
        { "findAllStepDefinitions", &FindAllStepDefinitions },
        { "findAllTestCaseFinished", &FindAllTestCaseFinished },
        { "findAllTestCaseFinishedOrderBy", &FindAllTestCaseFinishedOrderBy },
        { "findAllTestCases", &FindAllTestCases },
        { "findAllTestCaseStarted", &FindAllTestCaseStarted },
        { "findAllTestCaseStartedOrderBy", &FindAllTestCaseStartedOrderBy },
        { "findAllTestRunHookFinished", &FindAllTestRunHookFinished },

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

            // loadQuery
            {
                std::ifstream ifstream{ input };
                std::string line;
                while (std::getline(ifstream, line))
                {
                    cucumber::messages::Envelope envelope;
                    envelope.from_json(nlohmann::json::parse(line));

                    query.Update(envelope);
                }
            }

            {
                std::ifstream ifstream{ expected };
                functionMap.at(testCaseName)(query, nlohmann::json::parse(ifstream));
            }
        }

        cucumber::query::Query query;

        std::filesystem::path input;
        std::filesystem::path expected;
    };
}

GTEST_API_ int main(int argc, char** argv)
{
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
