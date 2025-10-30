using FluentAssertions;
using Io.Cucumber.Messages.Types;
using Io.Cucumber.Query;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace QueryTest
{
    [TestClass]
    public class QueryAcceptanceTest
    {
        private static readonly string[] _sources = new[]
        {
            "attachments.ndjson",
            "empty.ndjson",
            "examples-tables.ndjson",
            "global-hooks.ndjson",
            "global-hooks-attachments.ndjson",
            "hooks.ndjson",
            "minimal.ndjson",
            "rules.ndjson",
            "unknown-parameter-type.ndjson"
        };

        public static IEnumerable<object[]> Acceptance()
        {
            var queries = createQueries();
            foreach (var file in _sources)
            {
                foreach (var query in queries)
                {
                    yield return new object[] { new QueryTestCase(query.Key, file, query.Value) };
                }
            }
        }

        private static Dictionary<string, Func<Query, object>> createQueries()
        {
            var namingStrategy = NamingStrategy.Create(NamingStrategy.Strategy.LONG);
            var queries = new Dictionary<string, Func<Query, object>>
            {
                ["countMostSevereTestStepResultStatus"] = q => q.CountMostSevereTestStepResultStatus()
                                                                .ToDictionary(
                                                                    kvp => kvp.Key.EnumDescription<TestStepResultStatus>(),
                                                                    kvp => kvp.Value),
                ["countTestCasesStarted"] = q => q.TestCasesStartedCount,
                ["findAllPickles"] = q => q.FindAllPickles().Count(),
                ["findAllPickleSteps"] = q => q.FindAllPickleSteps().Count(),
                ["findAllStepDefinitions"] = q => q.FindAllStepDefinitions().Count(),
                ["findAllTestCaseStarted"] = q => q.FindAllTestCaseStarted().Count(),
                ["findAllTestCaseFinished"] = q => q.FindAllTestCaseFinished().Count(),
                ["findAllTestRunHookStarted"] = q => q.FindAllTestRunHookStarted().Count(),
                ["findAllTestRunHookFinished"] = q => q.FindAllTestRunHookFinished().Count(),
                ["findAllTestSteps"] = q => q.FindAllTestSteps().Count(),
                ["findAllTestStepsStarted"] = q => q.FindAllTestStepStarted().Count(),
                ["findAllTestStepsFinished"] = q => q.FindAllTestStepFinished().Count(),
                ["findAllTestCases"] = q => q.FindAllTestCases().Count(),
                ["findAllUndefinedParameterTypes"] = q => q.FindAllUndefinedParameterTypes()
                    .Select(upt => new object?[]
                        {
                            upt.Name,
                            upt.Expression
                        }).ToList(),
                ["findAttachmentsBy"] = q => new Dictionary<string, object>
                {
                    ["testStepFinished"] = q.FindAllTestCaseStarted()
                        .SelectMany(tcs => q.FindTestStepFinishedAndTestStepBy(tcs))
                        .Select(pair => pair.Item1)
                        .SelectMany(tsf => q.FindAttachmentsBy(tsf))
                        .Select(att => new object?[]
                        {
                            att.TestStepId,
                            att.TestCaseStartedId,
                            att.MediaType,
                            att.ContentEncoding
                        }).ToList(),
                    ["testRunHookFinished"] = q.FindAllTestRunHookFinished()
                        .SelectMany(trhf => q.FindAttachmentsBy(trhf))
                        .Select(att => new object?[]
                        {
                            att.TestRunHookStartedId,
                            att.MediaType,
                            att.ContentEncoding
                        }).ToList()
                },
                ["findHookBy"] = q => new Dictionary<string, object>
                {
                    ["testStep"] = q.FindAllTestSteps()
                        .Select(ts => q.FindHookBy(ts)?.Id)
                        .Where(id => id != null)
                        .ToList(),
                    ["testRunHookStarted"] = q.FindAllTestRunHookStarted()
                        .Select(trhs => q.FindHookBy(trhs)?.Id)
                        .Where(id => id != null)
                        .ToList(),
                    ["testRunHookFinished"] = q.FindAllTestRunHookFinished()
                        .Select(trhf => q.FindHookBy(trhf)?.Id)
                        .Where(id => id != null)
                        .ToList(),
                },
                ["findLineageBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => q.FindLineageBy(tcs))
                        .Where(lineage => lineage != null)
                        .Select(lineage => namingStrategy.Reduce(lineage))
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcs => q.FindLineageBy(tcs))
                        .Where(lineage => lineage != null)
                        .Select(lineage => namingStrategy.Reduce(lineage))
                        .ToList(),
                    ["pickle"] = q.FindAllPickles()
                        .Select(pickle => q.FindLineageBy(pickle))
                        .Where(lineage => lineage != null)
                        .Select(lineage => namingStrategy.Reduce(lineage))
                        .ToList()
                },
                ["findLocationOf"] = q => q.FindAllPickles()
                    .Select(pickle => q.FindLocationOf(pickle))
                    .Where(loc => loc != null)
                    .ToList(),
                ["findMeta"] = q => q.FindMeta()?.Implementation?.Name!,
                ["findMostSevereTestStepResultBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => q.FindMostSevereTestStepResultBy(tcs)?.Status)
                        .Where(status => status != null)
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => q.FindMostSevereTestStepResultBy(tcf)?.Status)
                        .Where(status => status != null)
                        .ToList()
                },
                ["findPickleBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => q.FindPickleBy(tcs)?.Name)
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => q.FindPickleBy(tcf)?.Name)
                        .ToList(),
                    ["testStepStarted"] = q.FindAllTestStepStarted()
                        .Select(tss => q.FindPickleBy(tss)?.Name)
                        .ToList(),
                    ["testStepFinished"] = q.FindAllTestStepFinished()
                        .Select(tsf => q.FindPickleBy(tsf)?.Name)
                        .ToList()
                },
                ["findPickleStepBy"] = q => q.FindAllTestSteps()
                    .Select(ts => q.FindPickleStepBy(ts)?.Text)
                    .Where(text => text != null)
                    .ToList(),
                ["findStepBy"] = q => q.FindAllPickleSteps()
                    .Select(ps => q.FindStepBy(ps)?.Text)
                    .ToList(),
                ["findStepDefinitionsBy"] = q => q.FindAllTestSteps()
                    .Select(ts => q.FindStepDefinitionsBy(ts).Select(sd => sd.Id).ToList())
                    .ToList(),
                ["findSuggestionsBy"] = q => new Dictionary<string, object>
                {
                    ["pickleStep"] = q.FindAllPickleSteps()
                        .SelectMany(ps => q.FindSuggestionsBy(ps))
                        .Select(s => s.Id)
                        .ToList(),
                    ["pickle"] = q.FindAllPickles()
                        .SelectMany(pickle => q.FindSuggestionsBy(pickle))
                        .Select(s => s.Id)
                        .ToList()
                },
                ["findUnambiguousStepDefinitionBy"] = q => q.FindAllTestSteps()
                    .Select(ts => q.FindUnambiguousStepDefinitionBy(ts)?.Id)
                    .Where(id => id != null)
                    .ToList(),
                ["findTestCaseStartedBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => q.FindTestCaseStartedBy(tcf)?.Id)
                        .ToList(),
                    ["testStepStarted"] = q.FindAllTestStepStarted()
                        .Select(tss => q.FindTestCaseStartedBy(tss)?.Id)
                        .ToList(),
                    ["testStepFinished"] = q.FindAllTestStepFinished()
                        .Select(tsf => q.FindTestCaseStartedBy(tsf)?.Id)
                        .ToList()
                },
                ["findTestCaseBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => q.FindTestCaseBy(tcs)?.Id)
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => q.FindTestCaseBy(tcf)?.Id)
                        .ToList(),
                    ["testStepStarted"] = q.FindAllTestStepStarted()
                        .Select(tss => q.FindTestCaseBy(tss)?.Id)
                        .ToList(),
                    ["testStepFinished"] = q.FindAllTestStepFinished()
                        .Select(tsf => q.FindTestCaseBy(tsf)?.Id)
                        .ToList()
                },
                ["findTestCaseDurationBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => ConvertTimeSpanToTimestamp(q.FindTestCaseDurationBy(tcs)))
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => ConvertTimeSpanToTimestamp(q.FindTestCaseDurationBy(tcf)))
                        .ToList()
                },
                ["findTestCaseFinishedBy"] = q => q.FindAllTestCaseStarted()
                    .Select(tcs => q.FindTestCaseFinishedBy(tcs)?.TestCaseStartedId)
                    .ToList(),
                ["findTestRunDuration"] = q => ConvertTimeSpanToTimestamp(q.FindTestRunDuration())!,
                ["findTestRunFinished"] = q => q.FindTestRunFinished()!,
                ["findTestRunStarted"] = q => q.FindTestRunStarted()!,
                ["findTestStepBy"] = q => q.FindAllTestCaseStarted()
                    .SelectMany(tcs => q.FindTestStepsStartedBy(tcs))
                    .Select(tss => q.FindTestStepBy(tss)?.Id)
                    .ToList(),
                ["findTestStepsStartedBy"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .Select(tcs => q.FindTestStepsStartedBy(tcs).Select(tss => tss.TestStepId).ToList())
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .Select(tcf => q.FindTestStepsStartedBy(tcf).Select(tss => tss.TestStepId).ToList())
                        .ToList()
                },
                ["findTestRunHookFinishedBy"] = q => q.FindAllTestRunHookStarted()
                    .Select(trhs => q.FindTestRunHookFinishedBy(trhs)?.TestRunHookStartedId)
                    .ToList(),
                ["findTestRunHookStartedBy"] = q => q.FindAllTestRunHookFinished()
                    .Select(trhf => q.FindTestRunHookStartedBy(trhf)?.Id)
                    .ToList(),
                ["findTestStepByTestStepFinished"] = q => new Dictionary<string, object>
                {
                    ["testCaseStarted"] = q.FindAllTestCaseStarted()
                        .SelectMany(tcs => q.FindTestStepsFinishedBy(tcs))
                        .Select(tsf => q.FindTestStepBy(tsf)?.Id)
                        .ToList(),
                    ["testCaseFinished"] = q.FindAllTestCaseFinished()
                        .SelectMany(tcf => q.FindTestStepsFinishedBy(tcf))
                        .Select(tsf => q.FindTestStepBy(tsf)?.Id)
                        .ToList()
                },
                ["findTestStepsFinishedBy"] = q => q.FindAllTestCaseStarted()
                    .Select(tcs => q.FindTestStepsFinishedBy(tcs).Select(tsf => tsf.TestStepId).ToList())
                    .ToList(),
                ["findTestStepFinishedAndTestStepBy"] = q => q.FindAllTestCaseStarted()
                    .SelectMany(tcs => q.FindTestStepFinishedAndTestStepBy(tcs))
                    .Select(pair => new object?[] { pair.Item1.TestStepId, pair.Item2.Id })
                    .ToList()
            };
            return queries;
        }

        [TestMethod]
        [DynamicData(nameof(Acceptance), DynamicDataSourceType.Method)]
        public void Test(QueryTestCase testCase)
        {
            var actual = WriteQueryResults(testCase);
            var expected = ReadResourceAsString(testCase.ExpectedResourceName);

            // Compare as JSON for robust diff
            var actualJson = JsonNode.Parse(actual);
            var expectedJson = JsonNode.Parse(expected);

            actualJson!.ToJsonString().Should().Be(expectedJson!.ToJsonString(),
                $"Query results for {testCase.Name}.{testCase.MethodName} do not match expected results.");
        }

        private static string WriteQueryResults(QueryTestCase testCase)
        {
            using var inStream = ReadResourceAsStream(testCase.SourceResourceName);
            using var reader = new StreamReader(inStream, Encoding.UTF8);

            var repository = CreateRepository();

            // Read NDJSON lines and update _query
            string? line;
            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                var envelope = NdjsonSerializer.Deserialize<Envelope>(line);
                repository.Update(envelope);
            }

            var query = new Query(repository);
            var queryResults = testCase.Query(query);
            var options = new JsonSerializerOptions(NdjsonSerializer.JsonOptions);
            options.Converters.Add(new TimestampOrderedConverter());
            options.Converters.Add(new TestRunStartedOrderedConverter());
            options.Converters.Add(new TestRunFinishedOrderedConverter());

            return JsonSerializer.Serialize(queryResults, options);
        }

        private static Repository CreateRepository()
        {
            return Repository.CreateWithAllFeatures();
        }

        private static Timestamp? ConvertTimeSpanToTimestamp(TimeSpan? duration)
        {
            if (duration == null) return null;
            return new Timestamp(
                (long)duration.Value.TotalSeconds,
                (int)((duration.Value.Ticks % TimeSpan.TicksPerSecond) * 100)
            );
        }

        private static Stream ReadResourceAsStream(string resourceName)
        {
            var fullName = Path.Combine(TestFolderHelper.TestFolder, "src", resourceName);
            if (!File.Exists(fullName))
                throw new FileNotFoundException($"Resource {fullName} not found.");
            return File.OpenRead(fullName);
        }

        private static string ReadResourceAsString(string resourceName)
        {
            using var stream = ReadResourceAsStream(resourceName);
            using var reader = new StreamReader(stream, Encoding.UTF8);
            return reader.ReadToEnd();
        }

        public class QueryTestCase
        {
            public string SourceResourceName { get; }
            public string ExpectedResourceName { get; }
            public string Name { get; }
            public string MethodName { get; }
            public Func<Query, object> Query { get; }

            public QueryTestCase(string methodName, string source, Func<Query, object> query)
            {
                MethodName = methodName;
                Name = source.Substring(0, source.LastIndexOf(".ndjson", StringComparison.Ordinal));
                SourceResourceName = source;
                ExpectedResourceName = $"{Name}.{MethodName}.results.json";
                Query = query;
            }

            public override string ToString() => Name;
        }
    }
}