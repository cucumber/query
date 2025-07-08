using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Io.Cucumber.Messages.Types;
using Io.Cucumber.Query;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace QueryTest
{
    [TestClass]
    public class QueryAcceptanceTest
    {
        private static readonly string[] TestFiles = new[]
        {
            "attachments.feature.ndjson",
            "empty.feature.ndjson",
            "hooks.feature.ndjson",
            "minimal.feature.ndjson",
            "rules.feature.ndjson",
            "examples-tables.feature.ndjson"
        };

        public static IEnumerable<object[]> Acceptance()
        {
            foreach (var file in TestFiles)
            {
                yield return new object[] { new TestCase(file) };
            }
        }

        [TestMethod]
        [DynamicData(nameof(Acceptance), DynamicDataSourceType.Method)]
        public void Test(TestCase testCase)
        {
            var actual = WriteQueryResults(testCase);
            var expected = ReadResourceAsString(testCase.ExpectedResourceName);

            // Compare as JSON for robust diff
            var actualJson = JsonNode.Parse(actual);
            var expectedJson = JsonNode.Parse(expected);

            Assert.AreEqual(expectedJson.ToJsonString(), actualJson.ToJsonString());
        }

        private static string WriteQueryResults(TestCase testCase)
        {
            using var inStream = ReadResourceAsStream(testCase.SourceResourceName);
            using var reader = new StreamReader(inStream, Encoding.UTF8);
            var query = new Query();

            // Read NDJSON lines and update query
            string? line;
            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                var envelope = JsonSerializer.Deserialize<Envelope>(line);
                query.Update(envelope);
            }

            var queryResults = CreateQueryResults(query);
            var options = new JsonSerializerOptions
            {
                WriteIndented = true,
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
            };
            return JsonSerializer.Serialize(queryResults, options);
        }

        private static Dictionary<string, object?> CreateQueryResults(Query query)
        {
            var results = new Dictionary<string, object?>
            {
                ["countMostSevereTestStepResultStatus"] = query.CountMostSevereTestStepResultStatus(),
                ["countTestCasesStarted"] = query.TestCasesStartedCount,
                ["findAllPickles"] = query.FindAllPickles().Count,
                ["findAllPickleSteps"] = query.FindAllPickleSteps().Count,
                ["findAllTestCaseStarted"] = query.FindAllTestCaseStarted().Count,
                ["findAllTestSteps"] = query.FindAllTestSteps().Count,
                ["findAllTestCaseStartedGroupedByFeature"] = query.FindAllTestCaseStartedGroupedByFeature()
                    .Select(entry => new object[]
                    {
                        entry.Key?.Name,
                        entry.Value.Select(tcs => tcs.Id).ToList()
                    }).ToList(),
                ["findAttachmentsBy"] = query.FindAllTestCaseStarted()
                    .SelectMany(tcs => query.FindTestStepFinishedAndTestStepBy(tcs))
                    .Select(pair => pair.Item1)
                    .SelectMany(tsf => query.FindAttachmentsBy(tsf))
                    .Select(att => new object?[]
                    {
                        att.TestStepId,
                        att.TestCaseStartedId,
                        att.MediaType,
                        att.ContentEncoding
                    }).ToList(),
                ["findFeatureBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindFeatureBy(tcs)?.Name)
                    .ToList(),
                ["findHookBy"] = query.FindAllTestSteps()
                    .Select(ts => query.FindHookBy(ts)?.Id)
                    .Where(id => id != null)
                    .ToList(),
                ["findLocationOf"] = query.FindAllPickles()
                    .Select(pickle => query.FindLocationOf(pickle))
                    .Where(loc => loc != null)
                    .ToList(),
                ["findMeta"] = query.FindMeta()?.Implementation?.Name,
                ["findMostSevereTestStepResultBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindMostSevereTestStepResultBy(tcs)?.Status)
                    .ToList(),
                ["findNameOf"] = new Dictionary<string, object?>
                {
                    ["long"] = query.FindAllPickles().Select(p => query.FindNameOf(p, NamingStrategy.Create(Strategy.LONG).Build())).ToList(),
                    ["excludeFeatureName"] = query.FindAllPickles().Select(p => query.FindNameOf(p, NamingStrategy.Create(Strategy.LONG).FeatureName(FeatureName.EXCLUDE).Build())).ToList(),
                    ["longPickleName"] = query.FindAllPickles().Select(p => query.FindNameOf(p, NamingStrategy.Create(Strategy.LONG).ExampleName(ExampleName.PICKLE).Build())).ToList(),
                    ["short"] = query.FindAllPickles().Select(p => query.FindNameOf(p, NamingStrategy.Create(Strategy.SHORT).Build())).ToList(),
                    ["shortPickleName"] = query.FindAllPickles().Select(p => query.FindNameOf(p, NamingStrategy.Create(Strategy.SHORT).ExampleName(ExampleName.PICKLE).Build())).ToList(),
                },
                ["findPickleBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindPickleBy(tcs)?.Name)
                    .ToList(),
                ["findPickleStepBy"] = query.FindAllTestSteps()
                    .Select(ts => query.FindPickleStepBy(ts)?.Text)
                    .Where(text => text != null)
                    .ToList(),
                ["findStepBy"] = query.FindAllPickleSteps()
                    .Select(ps => query.FindStepBy(ps)?.Text)
                    .ToList(),
                ["findTestCaseBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindTestCaseBy(tcs)?.Id)
                    .ToList(),
                ["findTestCaseDurationBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindTestCaseDurationBy(tcs)?.ToString())
                    .ToList(),
                ["findTestCaseFinishedBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindTestCaseFinishedBy(tcs)?.TestCaseStartedId)
                    .ToList(),
                ["findTestRunDuration"] = query.FindTestRunDuration()?.ToString(),
                ["findTestRunFinished"] = query.FindTestRunFinished(),
                ["findTestRunStarted"] = query.FindTestRunStarted(),
                ["findTestStepByTestStepStarted"] = query.FindAllTestCaseStarted()
                    .SelectMany(tcs => query.FindTestStepsStartedBy(tcs))
                    .Select(tss => query.FindTestStepBy(tss)?.Id)
                    .ToList(),
                ["findTestStepByTestStepFinished"] = query.FindAllTestCaseStarted()
                    .SelectMany(tcs => query.FindTestStepsFinishedBy(tcs))
                    .Select(tsf => query.FindTestStepBy(tsf)?.Id)
                    .ToList(),
                ["findTestStepsFinishedBy"] = query.FindAllTestCaseStarted()
                    .Select(tcs => query.FindTestStepsFinishedBy(tcs).Select(tsf => tsf.TestStepId).ToList())
                    .ToList(),
                ["findTestStepFinishedAndTestStepBy"] = query.FindAllTestCaseStarted()
                    .SelectMany(tcs => query.FindTestStepFinishedAndTestStepBy(tcs))
                    .Select(pair => new object?[] { pair.Item1.TestStepId, pair.Item2.Id })
                    .ToList(),
            };
            return results;
        }

        private static Stream ReadResourceAsStream(string resourceName)
        {
            var assembly = Assembly.GetExecutingAssembly();
            var fullName = assembly.GetManifestResourceNames().FirstOrDefault(n => n.EndsWith(resourceName));
            if (fullName == null)
                throw new FileNotFoundException($"Resource {resourceName} not found.");
            return assembly.GetManifestResourceStream(fullName)!;
        }

        private static string ReadResourceAsString(string resourceName)
        {
            using var stream = ReadResourceAsStream(resourceName);
            using var reader = new StreamReader(stream, Encoding.UTF8);
            return reader.ReadToEnd();
        }

        public class TestCase
        {
            public string SourceResourceName { get; }
            public string ExpectedResourceName { get; }
            public string Name { get; }

            public TestCase(string ndjsonFile)
            {
                Name = ndjsonFile.Substring(0, ndjsonFile.LastIndexOf(".ndjson", StringComparison.Ordinal));
                SourceResourceName = $"QueryTest.Resources.{ndjsonFile}";
                ExpectedResourceName = $"QueryTest.Resources.{Name}.query-results.json";
            }

            public override string ToString() => Name;
        }
    }
}