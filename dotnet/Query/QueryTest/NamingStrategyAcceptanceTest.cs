using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using FluentAssertions;
using Io.Cucumber.Messages.Types;
using Io.Cucumber.Query;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace QueryTest
{
    [TestClass]
    public class NamingStrategyAcceptanceTest
    {
        private static readonly Dictionary<string, NamingStrategy> Strategies = new()
        {
            { "long", NamingStrategy.Create(NamingStrategy.Strategy.LONG).Build() },
            { "long-exclude-feature-name", NamingStrategy.Create(NamingStrategy.Strategy.LONG).FeatureName(NamingStrategy.FeatureName.EXCLUDE).Build() },
            { "long-with-pickle-name", NamingStrategy.Create(NamingStrategy.Strategy.LONG).ExampleName(NamingStrategy.ExampleName.PICKLE).Build() },
            { "long-with-pickle-name-if-parameterized", NamingStrategy.Create(NamingStrategy.Strategy.LONG).ExampleName(NamingStrategy.ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED).Build() },
            { "short", NamingStrategy.Create(NamingStrategy.Strategy.SHORT).Build() }
        };

        public static IEnumerable<object[]> Acceptance()
        {
            var sources = new[]
            {
                Path.Combine("..", "..", "..", "..", "..", "..", "testdata", "minimal.feature.ndjson"),
                Path.Combine("..", "..", "..", "..", "..", "..", "testdata", "rules.feature.ndjson"),
                Path.Combine("..", "..", "..", "..", "..", "..", "testdata", "examples-tables.feature.ndjson")
            };

            foreach (var source in sources)
            {
                foreach (var kvp in Strategies)
                {
                    yield return new object[] { new TestCase(source, kvp.Key, kvp.Value) };
                }
            }
        }

        [TestMethod]
        [DynamicData(nameof(Acceptance), DynamicDataSourceType.Method)]
        public void Test(TestCase testCase)
        {
            var actual = WriteResults(testCase, testCase.Strategy);
            var expected = File.ReadAllText(testCase.Expected, Encoding.UTF8);
            actual.Should().Be(expected, $"NamingStrategy results for {testCase} do not match expected results.");
        }

        // Disabled: Only for updating expected files
        // [TestMethod]
        // [DynamicData(nameof(Acceptance), DynamicDataSourceType.Method)]
        // [Ignore]
        public void UpdateExpectedQueryResultFiles(TestCase testCase)
        {
            using var outStream = File.Open(testCase.Expected, FileMode.Create, FileAccess.Write);
            WriteResults(testCase.Strategy, testCase, outStream);
        }

        private static string WriteResults(TestCase testCase, NamingStrategy strategy)
        {
            using var outStream = new MemoryStream();
            WriteResults(strategy, testCase, outStream);
            return Encoding.UTF8.GetString(outStream.ToArray());
        }

        private static void WriteResults(NamingStrategy strategy, TestCase testCase, Stream outStream)
        {
            using var inStream = File.OpenRead(testCase.Source);
            using var reader = new StreamReader(inStream, Encoding.UTF8);
            using var writer = new StreamWriter(outStream, Encoding.UTF8, leaveOpen: true);
            var query = new Query();

            string? line;
            while ((line = reader.ReadLine()) != null)
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                var envelope = NdjsonSerializer.Deserialize<Envelope>(line);
                query.Update(envelope);
            }

            foreach (var pickle in query.FindAllPickles())
            {
                var lineage = query.FindLineageBy(pickle);
                if (lineage != null)
                {
                    var name = strategy.Reduce(lineage, pickle);
                    if (name != null)
                        writer.WriteLine(name);
                }
            }
            writer.Flush();
        }

        public class TestCase
        {
            public string Source { get; }
            public NamingStrategy Strategy { get; }
            public string Expected { get; }
            public string Name { get; }
            public string StrategyName { get; }

            public TestCase(string source, string strategyName, NamingStrategy strategy)
            {
                Source = source;
                Strategy = strategy;
                StrategyName = strategyName;
                var fileName = Path.GetFileName(source);
                Name = fileName.Substring(0, fileName.LastIndexOf(".ndjson", StringComparison.Ordinal));
                Expected = Path.Combine(Path.GetDirectoryName(source)!, $"{Name}.naming-strategy.{strategyName}.txt");
            }

            public override string ToString() => $"{Name} -> {StrategyName}";
        }
    }
}