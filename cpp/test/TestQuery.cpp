#include "cucumber/messages/All.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/NamingStrategy.hpp"
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
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        auto CollectFiles(const std::filesystem::path& folder) -> std::set<std::filesystem::path, std::less<>>
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

        auto ReversePickleComparator(const std::shared_ptr<const messages::Pickle>& lhs, const std::shared_ptr<const messages::Pickle>& rhs) -> std::int32_t
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

        auto FindPickleBy(const query::Query& query, std::variant<std::shared_ptr<const messages::TestCaseStarted>, std::shared_ptr<const messages::TestCaseFinished>,
                                                         std::shared_ptr<const messages::TestStepStarted>, std::shared_ptr<const messages::TestStepFinished>>
                                                         element) -> std::optional<std::shared_ptr<const messages::Pickle>>
        {
            return query.FindPickleBy(std::move(element));
        };

        template<auto F>
        auto GetSizeOf(const query::Query& query) -> nlohmann::json
        {
            const auto actual = (query.*F)();
            return actual.size();
        }

        auto CountMostSevereTestStepResultStatus(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual;

            for (const auto& [status, count] : query.CountMostSevereTestStepResultStatus())
            {
                actual[to_string(status)] = count;
            }

            return actual;
        }

        auto CountTestCasesStarted(const query::Query& query) -> nlohmann::json
        {
            const auto actual = query.CountTestCasesStarted();
            return actual;
        }

        auto FindAllTestCaseFinishedOrderBy(const query::Query& query) -> nlohmann::json
        {
            const auto allResults = query.FindAllTestCaseFinishedOrderBy(&FindPickleBy, ReversePickleComparator);
            nlohmann::json actual;
            for (const auto& testCaseFinished : allResults)
            {
                actual.push_back(testCaseFinished->testCaseStartedId);
            }

            return actual;
        }

        auto FindAllTestCaseStartedOrderBy(const query::Query& query) -> nlohmann::json
        {
            const auto allResults = query.FindAllTestCaseStartedOrderBy(&FindPickleBy, ReversePickleComparator);
            nlohmann::json actual;
            for (const auto& testCaseStarted : allResults)
            {
                actual.push_back(testCaseStarted->id);
            }

            return actual;
        }

        auto FindAllUndefinedParameterTypes(query::Query const& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& undefinedParameterType : query.FindAllUndefinedParameterTypes())
            {
                actual.push_back({ undefinedParameterType->name, undefinedParameterType->expression });
            }

            return actual;
        }

        auto FindAttachmentsBy(const query::Query& query) -> nlohmann::json
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

            return actual;
        }

        auto FindHookBy(const query::Query& query) -> nlohmann::json
        {
            const auto findHookBy = [&query](const auto& testSteps)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& testStep : testSteps)
                {
                    const auto& hook = query.FindHookBy(testStep);
                    if (hook.has_value())
                    {
                        actual.push_back(hook.value()->id);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testStep"] = findHookBy(query.FindAllTestSteps());
            actual["testRunHookStarted"] = findHookBy(query.FindAllTestRunHookStarted());
            actual["testRunHookFinished"] = findHookBy(query.FindAllTestRunHookFinished());

            return actual;
        }

        auto FindLineageBy(const query::Query& query) -> nlohmann::json
        {
            const auto namingStrategy = CreateNamingStrategy(NamingStrategyLength::longName, NamingStrategyFeatureName::include, NamingStrategyExampleName::number);

            const auto findLineageBy = [&namingStrategy, &query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    const auto& lineageAndPickle = query.FindLineageBy(item);
                    if (lineageAndPickle.has_value())
                    {
                        const auto& [lineage, pickle] = lineageAndPickle.value();
                        actual.push_back(namingStrategy->Reduce(*lineage, *pickle));
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findLineageBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findLineageBy(query.FindAllTestCaseFinished());
            actual["pickle"] = findLineageBy(query.FindAllPickles());

            return actual;
        }

        auto FindLocationOf(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& pickle : query.FindAllPickles())
            {
                const auto location = query.FindLocationOf(pickle);
                if (location.has_value())
                {
                    actual.push_back({ { "line", location.value()->line }, { "column", location.value()->column } });
                }
            }

            return actual;
        }

        auto FindMeta(const query::Query& query) -> nlohmann::json
        {
            const auto meta = query.FindMeta();

            if (meta.has_value())
            {
                return meta.value()->implementation->name;
            }

            return {};
        }

        auto FindMostSevereTestStepResultBy(const query::Query& query) -> nlohmann::json
        {
            const auto findMostSevereTestStepResultBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    const auto mostSevereTestStepResult = query.FindMostSevereTestStepResultBy(item);
                    if (mostSevereTestStepResult.has_value())
                    {
                        actual.emplace_back(to_string(mostSevereTestStepResult.value()->status));
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findMostSevereTestStepResultBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findMostSevereTestStepResultBy(query.FindAllTestCaseFinished());

            return actual;
        }

        auto FindPickleBy(const query::Query& query) -> nlohmann::json
        {
            const auto findPickleBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    const auto pickle = query.FindPickleBy(item);
                    if (pickle.has_value())
                    {
                        actual.push_back(pickle.value()->name);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findPickleBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findPickleBy(query.FindAllTestCaseFinished());
            actual["testStepStarted"] = findPickleBy(query.FindAllTestStepStarted());
            actual["testStepFinished"] = findPickleBy(query.FindAllTestStepFinished());

            return actual;
        }

        auto FindPickleStepBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testStep : query.FindAllTestSteps())
            {
                const auto pickleStep = query.FindPickleStepBy(testStep);
                if (pickleStep.has_value())
                {
                    actual.emplace_back(pickleStep.value()->text);
                }
            }

            return actual;
        }

        auto FindStepBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& pickleStep : query.FindAllPickleSteps())
            {
                const auto step = query.FindStepBy(pickleStep);
                if (step.has_value())
                {
                    actual.emplace_back(step.value()->text);
                }
            }

            return actual;
        }

        auto FindStepDefinitionsBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testStep : query.FindAllTestSteps())
            {
                auto& actualIds = actual.emplace_back(nlohmann::json::array());

                for (const auto& stepDefinition : query.FindStepDefinitionsBy(testStep))
                {
                    actualIds.emplace_back(stepDefinition->id);
                }
            }

            return actual;
        }

        auto FindSuggestionsBy(const query::Query& query) -> nlohmann::json
        {
            const auto findSuggestionsBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    for (const auto& suggestion : query.FindSuggestionsBy(item))
                    {
                        actual.emplace_back(suggestion->id);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["pickleStep"] = findSuggestionsBy(query.FindAllPickleSteps());
            actual["pickle"] = findSuggestionsBy(query.FindAllPickles());

            return actual;
        }

        auto FindTestCaseBy(const query::Query& query) -> nlohmann::json
        {
            const auto findTestCaseBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    const auto& testCase = query.FindTestCaseBy(item);
                    if (testCase.has_value())
                    {
                        actual.push_back(testCase.value()->id);
                    }
                }
                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findTestCaseBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findTestCaseBy(query.FindAllTestCaseFinished());
            actual["testStepStarted"] = findTestCaseBy(query.FindAllTestStepStarted());
            actual["testStepFinished"] = findTestCaseBy(query.FindAllTestStepFinished());

            return actual;
        }

        auto FindTestCaseDurationBy(const query::Query& query) -> nlohmann::json
        {
            const auto findTestCaseDurationBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    const auto& testCase = query.FindTestCaseDurationBy(item);
                    if (testCase.has_value())
                    {
                        testCase.value()->to_json(actual.emplace_back());
                    }
                }
                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findTestCaseDurationBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findTestCaseDurationBy(query.FindAllTestCaseFinished());

            return actual;
        }

        auto FindTestCaseFinishedBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testCaseStarted : query.FindAllTestCaseStarted())
            {
                const auto& testCaseFinished = query.FindTestCaseFinishedBy(testCaseStarted);
                if (testCaseFinished.has_value())
                {
                    actual.push_back(testCaseFinished.value()->testCaseStartedId);
                }
            }

            return actual;
        }

        auto FindTestCaseStartedBy(const query::Query& query) -> nlohmann::json
        {
            const auto findTestCaseStartedBy = [&query](const auto& items)
            {
                nlohmann::json actual = nlohmann::json::array();
                for (const auto& testCaseFinished : items)
                {
                    const auto& testCaseStarted = query.FindTestCaseStartedBy(testCaseFinished);
                    if (testCaseStarted.has_value())
                    {
                        actual.push_back(testCaseStarted.value()->id);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseFinished"] = findTestCaseStartedBy(query.FindAllTestCaseFinished());
            actual["testStepStarted"] = findTestCaseStartedBy(query.FindAllTestStepStarted());
            actual["testStepFinished"] = findTestCaseStartedBy(query.FindAllTestStepFinished());

            return actual;
        }

        auto FindTestRunDuration(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual;

            const auto testRunDuration = query.FindTestRunDuration();
            if (testRunDuration.has_value())
            {
                testRunDuration.value()->to_json(actual);
            }

            return actual;
        }

        auto FindTestRunFinished(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual;

            const auto testRunFinished = query.FindTestRunFinished();
            if (testRunFinished.has_value())
            {
                testRunFinished.value()->to_json(actual);
            }

            return actual;
        }

        auto FindTestRunHookFinishedBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testRunHookStarted : query.FindAllTestRunHookStarted())
            {
                const auto& testRunHookFinished = query.FindTestRunHookFinishedBy(testRunHookStarted);
                if (testRunHookFinished.has_value())
                {
                    actual.emplace_back(testRunHookFinished.value()->testRunHookStartedId);
                }
            }

            return actual;
        }

        auto FindTestRunHookStartedBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testRunHookFinished : query.FindAllTestRunHookFinished())
            {
                const auto& testRunHookStarted = query.FindTestRunHookStartedBy(testRunHookFinished);
                if (testRunHookStarted.has_value())
                {
                    actual.emplace_back(testRunHookStarted.value()->id);
                }
            }

            return actual;
        }

        auto FindTestRunStarted(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual;

            const auto testRunStarted = query.FindTestRunStarted();
            if (testRunStarted.has_value())
            {
                testRunStarted.value()->to_json(actual);
            }

            return actual;
        }

        auto FindTestStepBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual{
                { "testStepStarted", nlohmann::json::array() },
                { "testStepFinished", nlohmann::json::array() },
            };

            const auto findTestStepBy = [&](const auto& testSteps, const char* key)
            {
                for (const auto& testStepEvent : testSteps)
                {
                    const auto& testStep = query.FindTestStepBy(testStepEvent);
                    if (testStep.has_value())
                    {
                        actual[key].emplace_back(testStep.value()->id);
                    }
                }
            };

            for (const auto& item : query.FindAllTestCaseStarted())
            {
                findTestStepBy(query.FindTestStepsStartedBy(item), "testStepStarted");
                findTestStepBy(query.FindTestStepsFinishedBy(item), "testStepFinished");
            }

            return actual;
        }

        auto FindTestStepFinishedAndTestStepBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testCaseStarted : query.FindAllTestCaseStarted())
            {
                for (const auto& [testStepFinished, testStep] : query.FindTestStepFinishedAndTestStepBy(testCaseStarted))
                {
                    actual.push_back({ testStepFinished->testStepId, testStep->id });
                }
            }

            return actual;
        }

        auto FindTestStepsFinishedBy(const query::Query& query) -> nlohmann::json
        {
            const auto findTestStepsFinishedBy = [&](const auto& items)
            {
                auto actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    auto& nested = actual.emplace_back(nlohmann::json::array());
                    const auto& testStepsFinished = query.FindTestStepsFinishedBy(item);

                    for (const auto& testStepFinished : testStepsFinished)
                    {
                        nested.push_back(testStepFinished->testStepId);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findTestStepsFinishedBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findTestStepsFinishedBy(query.FindAllTestCaseFinished());

            return actual;
        }

        auto FindTestStepsStartedBy(const query::Query& query) -> nlohmann::json
        {
            const auto findTestStepsStartedBy = [&](const auto& items)
            {
                auto actual = nlohmann::json::array();

                for (const auto& item : items)
                {
                    auto& nested = actual.emplace_back(nlohmann::json::array());
                    const auto& testStepsFinished = query.FindTestStepsStartedBy(item);

                    for (const auto& testStepFinished : testStepsFinished)
                    {
                        nested.push_back(testStepFinished->testStepId);
                    }
                }

                return actual;
            };

            nlohmann::json actual;

            actual["testCaseStarted"] = findTestStepsStartedBy(query.FindAllTestCaseStarted());
            actual["testCaseFinished"] = findTestStepsStartedBy(query.FindAllTestCaseFinished());

            return actual;
        }

        auto FindUnambiguousStepDefinitionBy(const query::Query& query) -> nlohmann::json
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testStep : query.FindAllTestSteps())
            {
                const auto& stepDefinition = query.FindUnambiguousStepDefinitionBy(testStep);
                if (stepDefinition.has_value())
                {
                    actual.push_back(stepDefinition.value()->id);
                }
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
            { "findHookBy", FindHookBy },
            { "findLineageBy", FindLineageBy },
            { "findLocationOf", FindLocationOf },
            { "findMeta", FindMeta },
            { "findMostSevereTestStepResultBy", FindMostSevereTestStepResultBy },
            { "findPickleBy", FindPickleBy },
            { "findPickleStepBy", FindPickleStepBy },
            { "findStepBy", FindStepBy },
            { "findStepDefinitionsBy", FindStepDefinitionsBy },
            { "findSuggestionsBy", FindSuggestionsBy },
            { "findTestCaseBy", FindTestCaseBy },
            { "findTestCaseDurationBy", FindTestCaseDurationBy },
            { "findTestCaseFinishedBy", FindTestCaseFinishedBy },
            { "findTestCaseStartedBy", FindTestCaseStartedBy },
            { "findTestRunDuration", FindTestRunDuration },
            { "findTestRunFinished", FindTestRunFinished },
            { "findTestRunHookFinishedBy", FindTestRunHookFinishedBy },
            { "findTestRunHookStartedBy", FindTestRunHookStartedBy },
            { "findTestRunStarted", FindTestRunStarted },
            { "findTestStepBy", FindTestStepBy },
            { "findTestStepFinishedAndTestStepBy", FindTestStepFinishedAndTestStepBy },
            { "findTestStepsFinishedBy", FindTestStepsFinishedBy },
            { "findTestStepsStartedBy", FindTestStepsStartedBy },
            { "findUnambiguousStepDefinitionBy", FindUnambiguousStepDefinitionBy },
        };

        struct AcceptanceTest : testing::Test
        {
            AcceptanceTest(std::filesystem::path input, std::filesystem::path expected)
                : input{ std::move(input) }
                , expected{ std::move(expected) }
            {}

        private:
            auto TestBody() -> void override
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

        auto GetTestFiles() -> TestData
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

        auto FilterName(std::string str) -> std::string
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
