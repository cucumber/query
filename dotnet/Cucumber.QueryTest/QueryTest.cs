using Io.Cucumber.Messages.Types;
using Cucumber.Query;
using System.Text;

namespace Cucumber.QueryTest;

[TestClass]
public class QueryTest
{
    private readonly Repository _repository;
    private readonly Cucumber.Query.Query _query;

    public QueryTest()
    {
        _repository = new Repository();
        _query = new Cucumber.Query.Query(_repository);
    }

    [TestMethod]
    public void RetainsInsertionOrderForTestCaseStarted()
    {
        var a = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(1L, 0L));
        var b = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(1L, 0L));
        var c = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(1L, 0L));

        foreach (var tcs in new[] { a, b, c })
            _repository.UpdateTestCaseStarted(tcs);

        var result = _query.FindAllTestCaseStarted();
        CollectionAssert.AreEqual(new[] { a, b, c }, result.ToArray());
    }

    [TestMethod]
    public void OmitsTestCaseStartedIfFinishedAndWillBeRetried()
    {
        var a = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(0L, 0L));
        var b = new TestCaseFinished(a.Id, new Timestamp(0L, 0L), true);
        var c = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(0L, 0L));
        var d = new TestCaseFinished(c.Id, new Timestamp(0L, 0L), false);

        _repository.UpdateTestCaseStarted(a);
        _repository.UpdateTestCaseStarted(c);
        _repository.UpdateTestCaseFinished(b);
        _repository.UpdateTestCaseFinished(d);

        var result = _query.FindAllTestCaseStarted();
        Assert.AreEqual(1, result.Count());
        Assert.AreEqual(c, result.ToArray()[0]);
        Assert.AreEqual(1, _query.TestCasesStartedCount);
    }

    [TestMethod]
    public void FindLocationOfFallsBackToScenarioLocationWhenPickleHasNoLocation()
    {
        var envelopes = LoadEnvelopes("minimal.ndjson");
        var query = BuildQuery(envelopes);
        var pickle = FindPickle(envelopes);

        Assert.AreEqual(pickle.Location, query.FindLocationOf(StripLocation(pickle)));
    }

    [TestMethod]
    public void FindLocationOfFallsBackToExampleRowLocationWhenPickleHasNoLocation()
    {
        var envelopes = LoadEnvelopes("examples-tables.ndjson");
        var query = BuildQuery(envelopes);
        var pickle = FindPickle(envelopes);

        Assert.AreEqual(pickle.Location, query.FindLocationOf(StripLocation(pickle)));
    }

    private static Cucumber.Query.Query BuildQuery(IEnumerable<Envelope> envelopes)
    {
        var repository = Repository.CreateWithAllFeatures();
        foreach (var envelope in envelopes)
            repository.Update(envelope);
        return new Cucumber.Query.Query(repository);
    }

    private static Pickle FindPickle(IEnumerable<Envelope> envelopes) =>
        envelopes.First(e => e.Pickle != null).Pickle;

    private static Pickle StripLocation(Pickle pickle) =>
        new Pickle(pickle.Id, pickle.Uri, null, pickle.Name, pickle.Language, pickle.Steps,
            pickle.Tags, pickle.AstNodeIds);

    private static IReadOnlyList<Envelope> LoadEnvelopes(string source)
    {
        var path = Path.Combine(TestFolderHelper.TestFolder, "src", source);
        return File.ReadAllLines(path, Encoding.UTF8)
            .Where(line => !string.IsNullOrWhiteSpace(line))
            .Select(NdjsonSerializer.Deserialize)
            .ToList();
    }

    private static string RandomId() => Guid.NewGuid().ToString();
}
