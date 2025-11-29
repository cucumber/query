using FluentAssertions;
using Io.Cucumber.Messages.Types;
using Cucumber.Query;
using System.Text;

namespace Cucumber.QueryTest;

[TestClass]
public class NamingStrategyAcceptanceTest
{
    private static readonly Dictionary<string, NamingStrategy> Strategies = new()
    {
        { "long", NamingStrategy.Create(NamingStrategy.Strategy.LONG) },
        { "long-exclude-feature-name", NamingStrategy.Create(NamingStrategy.Strategy.LONG, NamingStrategy.FeatureName.EXCLUDE) },
        { "long-with-pickle-name", NamingStrategy.Create(NamingStrategy.Strategy.LONG, NamingStrategy.ExampleName.PICKLE) },
        { "long-with-pickle-name-if-parameterized", NamingStrategy.Create(NamingStrategy.Strategy.LONG, NamingStrategy.ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED)},
        { "short", NamingStrategy.Create(NamingStrategy.Strategy.SHORT) }
    };

    public static IEnumerable<object[]> Acceptance()
    {
        var testDataPath = TestFolderHelper.TestFolder;
        var sources = new[]
        {
            Path.Combine(testDataPath, "src", "minimal.ndjson"),
            Path.Combine(testDataPath, "src", "rules.ndjson"),
            Path.Combine(testDataPath, "src", "examples-tables.ndjson")
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
        using var writer = new StreamWriter(outStream, new UTF8Encoding(false), leaveOpen: true);

        var repository = CreateRepository();

        string? line;
        while ((line = reader.ReadLine()) != null)
        {
            if (string.IsNullOrWhiteSpace(line)) continue;
            var envelope = NdjsonSerializer.Deserialize<Envelope>(line);
            repository.Update(envelope);
        }
        var query = new Cucumber.Query.Query(repository);

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

    private static Repository CreateRepository() => new Repository(new[] { Repository.RepositoryFeature.INCLUDE_GHERKIN_DOCUMENTS });

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
