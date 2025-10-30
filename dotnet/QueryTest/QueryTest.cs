using Io.Cucumber.Messages.Types;
using Io.Cucumber.Query;

namespace QueryTest
{
    [TestClass]
    public class QueryTest
    {
        private readonly Repository _repository;
        private readonly Query _query;

        public QueryTest()
        {
            _repository = new Repository();
            _query = new Query(_repository);
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

        private static string RandomId() => Guid.NewGuid().ToString();
    }
}
