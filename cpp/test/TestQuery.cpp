#include "cucumber/messages/All.hpp"
#include "cucumber/messages/TestStepResultStatus.hpp"
#include "cucumber/query/NamingStrategy.hpp"
#include "cucumber/query/Query.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cucumber::query
{
    namespace
    {
        auto EndsWith(std::string_view value, std::string_view suffix) -> bool
        {
            return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        struct Expectation
        {
            std::string_view infix;
            std::string_view suffix;
        };

        struct DataSet
        {
            std::filesystem::path source;
        };

        // Determines how ctest names the generated tests.
        auto PrintTo(const DataSet& dataSet, std::ostream* stream) -> void
        {
            *stream << dataSet.source.stem().string();
        }

        // Data sets are `<stem>.ndjson`, their expectations `<stem><infix><discriminator><suffix>`.
        auto GetDataSets(const Expectation& expectation) -> std::vector<DataSet>
        {
            std::vector<std::filesystem::path> sources;
            std::vector<std::string> expectations;

            for (const auto& entry : std::filesystem::directory_iterator{ std::filesystem::path{ TESTDATA_SRC } })
            {
                auto fileName = entry.path().filename().string();

                if (entry.path().extension() == ".ndjson")
                {
                    sources.push_back(entry.path());
                }
                else if (EndsWith(fileName, expectation.suffix))
                {
                    expectations.push_back(std::move(fileName));
                }
            }

            std::vector<DataSet> dataSets;

            for (auto& source : sources)
            {
                const auto prefix = source.stem().string() + std::string{ expectation.infix };
                const auto hasExpectations = std::any_of(expectations.begin(), expectations.end(),
                    [&prefix](const std::string& expectation) -> bool
                    {
                        return expectation.compare(0, prefix.size(), prefix) == 0;
                    });

                if (hasExpectations)
                {
                    dataSets.push_back(DataSet{ std::move(source) });
                }
            }

            std::sort(dataSets.begin(), dataSets.end(),
                [](const DataSet& lhs, const DataSet& rhs) -> bool
                {
                    return lhs.source < rhs.source;
                });

            return dataSets;
        }

        auto ExpectedPath(const std::filesystem::path& dataSet, const std::string& suffix) -> std::filesystem::path
        {
            return dataSet.parent_path() / (dataSet.stem().string() + suffix);
        }

        auto DataSetName(const testing::TestParamInfo<DataSet>& info) -> std::string
        {
            auto name = info.param.source.stem().string();

            std::replace_if(
                name.begin(), name.end(),
                [](unsigned char character) -> bool
                {
                    return std::isalnum(character) == 0;
                },
                '_');

            return name;
        }

        auto LoadQuery(const std::filesystem::path& source) -> Query
        {
            Query query;

            std::ifstream ifstream{ source };
            std::string line;
            while (std::getline(ifstream, line))
            {
                messages::Envelope envelope;
                envelope.from_json(nlohmann::json::parse(line));

                query.Update(envelope);
            }

            return query;
        }

        auto ReversePickleComparator(const std::shared_ptr<const messages::Pickle>& lhs, const std::shared_ptr<const messages::Pickle>& rhs) -> std::int32_t
        {
            if (lhs->uri != rhs->uri)
            {
                return static_cast<std::int32_t>(lhs->uri.compare(rhs->uri));
            }
            if (!lhs->location.has_value() || !rhs->location.has_value())
            {
                return 0;
            }
            if (lhs->location.value()->line != rhs->location.value()->line)
            {
                return static_cast<std::int32_t>(rhs->location.value()->line) - static_cast<std::int32_t>(lhs->location.value()->line);
            }
            return static_cast<std::int32_t>(rhs->location.value()->column.value_or(0)) - static_cast<std::int32_t>(lhs->location.value()->column.value_or(0));
        }

        struct QueryAcceptanceTest : testing::TestWithParam<DataSet>
        {
        protected:
            auto SetUp() -> void override
            {
                query = LoadQuery(GetParam().source);
            }

            static auto Verify(std::string_view queryName, const nlohmann::json& actual) -> void
            {
                const auto expected = ExpectedPath(GetParam().source, "." + std::string{ queryName } + ".results.json");

                if (!std::filesystem::exists(expected))
                {
                    GTEST_SKIP() << "No expected results at " << expected;
                }

                std::ifstream ifstream{ expected };
                EXPECT_THAT(actual, testing::Eq(nlohmann::json::parse(ifstream)));
            }

            Query query;
        };

        INSTANTIATE_TEST_SUITE_P(Acceptance, QueryAcceptanceTest, testing::ValuesIn(GetDataSets({ ".", ".results.json" })), DataSetName);

        TEST_P(QueryAcceptanceTest, findAllPickles)
        {
            Verify("findAllPickles", query.FindAllPickles().size());
        }

        TEST_P(QueryAcceptanceTest, findAllPickleSteps)
        {
            Verify("findAllPickleSteps", query.FindAllPickleSteps().size());
        }

        TEST_P(QueryAcceptanceTest, findAllStepDefinitions)
        {
            Verify("findAllStepDefinitions", query.FindAllStepDefinitions().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestCaseFinished)
        {
            Verify("findAllTestCaseFinished", query.FindAllTestCaseFinished().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestCases)
        {
            Verify("findAllTestCases", query.FindAllTestCases().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestCaseStarted)
        {
            Verify("findAllTestCaseStarted", query.FindAllTestCaseStarted().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestRunHookFinished)
        {
            Verify("findAllTestRunHookFinished", query.FindAllTestRunHookFinished().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestRunHookStarted)
        {
            Verify("findAllTestRunHookStarted", query.FindAllTestRunHookStarted().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestStepFinished)
        {
            Verify("findAllTestStepFinished", query.FindAllTestStepFinished().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestSteps)
        {
            Verify("findAllTestSteps", query.FindAllTestSteps().size());
        }

        TEST_P(QueryAcceptanceTest, findAllTestStepStarted)
        {
            Verify("findAllTestStepStarted", query.FindAllTestStepStarted().size());
        }

        TEST_P(QueryAcceptanceTest, countMostSevereTestStepResultStatus)
        {
            nlohmann::json actual;

            for (const auto& [status, count] : query.CountMostSevereTestStepResultStatus())
            {
                actual[to_string(status)] = count;
            }

            Verify("countMostSevereTestStepResultStatus", actual);
        }

        TEST_P(QueryAcceptanceTest, countTestCasesStarted)
        {
            Verify("countTestCasesStarted", query.CountTestCasesStarted());
        }

        TEST_P(QueryAcceptanceTest, findAllTestCaseFinishedOrderBy)
        {
            const auto allResults = query.FindAllTestCaseFinishedOrderBy(
                [](const query::Query& query, const auto& element)
                {
                    return query.FindPickleBy(element);
                },
                ReversePickleComparator);

            nlohmann::json actual;
            for (const auto& testCaseFinished : allResults)
            {
                actual.push_back(testCaseFinished->testCaseStartedId);
            }

            Verify("findAllTestCaseFinishedOrderBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findAllTestCaseStartedOrderBy)
        {
            const auto allResults = query.FindAllTestCaseStartedOrderBy(
                [](const query::Query& query, const auto& element)
                {
                    return query.FindPickleBy(element);
                },
                ReversePickleComparator);

            nlohmann::json actual;
            for (const auto& testCaseStarted : allResults)
            {
                actual.push_back(testCaseStarted->id);
            }

            Verify("findAllTestCaseStartedOrderBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findAllUndefinedParameterTypes)
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& undefinedParameterType : query.FindAllUndefinedParameterTypes())
            {
                actual.push_back({ undefinedParameterType->name, undefinedParameterType->expression });
            }

            Verify("findAllUndefinedParameterTypes", actual);
        }

        TEST_P(QueryAcceptanceTest, findAttachmentsBy)
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

            Verify("findAttachmentsBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findHookBy)
        {
            const auto findHookBy = [this](const auto& testSteps)
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

            Verify("findHookBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findLineageBy)
        {
            const auto namingStrategy = CreateNamingStrategy(NamingStrategyLength::longName, NamingStrategyFeatureName::include, NamingStrategyExampleName::number);

            const auto findLineageBy = [this, &namingStrategy](const auto& items)
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

            Verify("findLineageBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findLocationOf)
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

            Verify("findLocationOf", actual);
        }

        TEST_P(QueryAcceptanceTest, findMeta)
        {
            const auto meta = query.FindMeta();

            nlohmann::json actual;
            if (meta.has_value())
            {
                actual = meta.value()->implementation->name;
            }

            Verify("findMeta", actual);
        }

        TEST_P(QueryAcceptanceTest, findMostSevereTestStepResultBy)
        {
            const auto findMostSevereTestStepResultBy = [this](const auto& items)
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

            Verify("findMostSevereTestStepResultBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findPickleBy)
        {
            const auto findPickleBy = [this](const auto& items)
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

            Verify("findPickleBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findPickleStepBy)
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

            Verify("findPickleStepBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findStepBy)
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

            Verify("findStepBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findStepDefinitionsBy)
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

            Verify("findStepDefinitionsBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findSuggestionsBy)
        {
            const auto findSuggestionsBy = [this](const auto& items)
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

            Verify("findSuggestionsBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestCaseBy)
        {
            const auto findTestCaseBy = [this](const auto& items)
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

            Verify("findTestCaseBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestCaseDurationBy)
        {
            const auto findTestCaseDurationBy = [this](const auto& items)
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

            Verify("findTestCaseDurationBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestCaseFinishedBy)
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

            Verify("findTestCaseFinishedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestCaseStartedBy)
        {
            const auto findTestCaseStartedBy = [this](const auto& items)
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

            Verify("findTestCaseStartedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestRunDuration)
        {
            nlohmann::json actual;

            const auto testRunDuration = query.FindTestRunDuration();
            if (testRunDuration.has_value())
            {
                testRunDuration.value()->to_json(actual);
            }

            Verify("findTestRunDuration", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestRunFinished)
        {
            nlohmann::json actual;

            const auto testRunFinished = query.FindTestRunFinished();
            if (testRunFinished.has_value())
            {
                testRunFinished.value()->to_json(actual);
            }

            Verify("findTestRunFinished", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestRunHookFinishedBy)
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

            Verify("findTestRunHookFinishedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestRunHookStartedBy)
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

            Verify("findTestRunHookStartedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestRunStarted)
        {
            nlohmann::json actual;

            const auto testRunStarted = query.FindTestRunStarted();
            if (testRunStarted.has_value())
            {
                testRunStarted.value()->to_json(actual);
            }

            Verify("findTestRunStarted", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestStepBy)
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

            Verify("findTestStepBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestStepFinishedAndTestStepBy)
        {
            nlohmann::json actual = nlohmann::json::array();

            for (const auto& testCaseStarted : query.FindAllTestCaseStarted())
            {
                for (const auto& [testStepFinished, testStep] : query.FindTestStepFinishedAndTestStepBy(testCaseStarted))
                {
                    actual.push_back({ testStepFinished->testStepId, testStep->id });
                }
            }

            Verify("findTestStepFinishedAndTestStepBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestStepsFinishedBy)
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

            Verify("findTestStepsFinishedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findTestStepsStartedBy)
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

            Verify("findTestStepsStartedBy", actual);
        }

        TEST_P(QueryAcceptanceTest, findUnambiguousStepDefinitionBy)
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

            Verify("findUnambiguousStepDefinitionBy", actual);
        }

        struct NamingStrategyAcceptanceTest : testing::TestWithParam<DataSet>
        {
        protected:
            auto SetUp() -> void override
            {
                query = LoadQuery(GetParam().source);
            }

            auto Verify(std::string_view variant, const NamingStrategy& strategy) const -> void
            {
                std::string actual;

                for (const auto& pickle : query.FindAllPickles())
                {
                    const auto lineageAndPickle = query.FindLineageBy(pickle);
                    if (lineageAndPickle.has_value())
                    {
                        actual += strategy.Reduce(*lineageAndPickle.value().lineage, *lineageAndPickle.value().pickle) + "\n";
                    }
                }

                const auto expected = ExpectedPath(GetParam().source, ".naming-strategy." + std::string{ variant } + ".txt");

                std::ifstream ifstream{ expected, std::ios::binary };
                EXPECT_EQ(actual, std::string(std::istreambuf_iterator<char>{ ifstream }, std::istreambuf_iterator<char>{}));
            }

            Query query;
        };

        INSTANTIATE_TEST_SUITE_P(Acceptance, NamingStrategyAcceptanceTest, testing::ValuesIn(GetDataSets({ ".naming-strategy.", ".txt" })), DataSetName);

        TEST_P(NamingStrategyAcceptanceTest, long)
        {
            Verify("long", *CreateNamingStrategy(NamingStrategyLength::longName));
        }

        TEST_P(NamingStrategyAcceptanceTest, long_exclude_feature_name)
        {
            Verify("long-exclude-feature-name", *CreateNamingStrategy(NamingStrategyLength::longName, NamingStrategyFeatureName::exclude));
        }

        TEST_P(NamingStrategyAcceptanceTest, long_with_pickle_name)
        {
            Verify("long-with-pickle-name", *CreateNamingStrategy(NamingStrategyLength::longName, NamingStrategyFeatureName::include, NamingStrategyExampleName::pickle));
        }

        TEST_P(NamingStrategyAcceptanceTest, long_with_pickle_name_if_parameterized)
        {
            Verify("long-with-pickle-name-if-parameterized",
                *CreateNamingStrategy(NamingStrategyLength::longName, NamingStrategyFeatureName::include, NamingStrategyExampleName::numberAndPickleIfParameterized));
        }

        TEST_P(NamingStrategyAcceptanceTest, short)
        {
            Verify("short", *CreateNamingStrategy(NamingStrategyLength::shortName));
        }
    }
}
