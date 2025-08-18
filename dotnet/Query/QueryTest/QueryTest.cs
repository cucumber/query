using System;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Io.Cucumber.Query;
using Io.Cucumber.Messages.Types;
using System.Collections.Generic;

namespace QueryTest
{
    [TestClass]
    public class QueryTest
    {
        private readonly Query query = new Query();

        [TestMethod]
        public void RetainsTimestampOrderForTestCaseStarted()
        {
            var a = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(1L, 0L));
            var b = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(2L, 0L));
            var c = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(3L, 0L));

            foreach (var tcs in new[] { a, b, c })
                query.UpdateTestCaseStarted(tcs);

            var result = query.FindAllTestCaseStarted();
            CollectionAssert.AreEqual(new[] { a, b, c }, result.ToArray());
        }

        [TestMethod]
        public void IdIsTieOrderTieBreaker()
        {
            var a = new TestCaseStarted(0L, "2", RandomId(), "main", new Timestamp(1L, 0L));
            var b = new TestCaseStarted(0L, "1", RandomId(), "main", new Timestamp(1L, 0L));
            var c = new TestCaseStarted(0L, "0", RandomId(), "main", new Timestamp(1L, 0L));

            foreach (var tcs in new[] { a, b, c })
                query.UpdateTestCaseStarted(tcs);

            var result = query.FindAllTestCaseStarted();
            CollectionAssert.AreEqual(new[] { c, b, a }, result.ToArray());
        }

        [TestMethod]
        public void OmitsTestCaseStartedIfFinishedAndWillBeRetried()
        {
            var a = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(0L, 0L));
            var b = new TestCaseFinished(a.Id, new Timestamp(0L, 0L), true);
            var c = new TestCaseStarted(0L, RandomId(), RandomId(), "main", new Timestamp(0L, 0L));
            var d = new TestCaseFinished(c.Id, new Timestamp(0L, 0L), false);

            query.UpdateTestCaseStarted(a);
            query.UpdateTestCaseStarted(c);
            query.UpdateTestCaseFinished(b);
            query.UpdateTestCaseFinished(d);

            var result = query.FindAllTestCaseStarted();
            Assert.AreEqual(1, result.Count);
            Assert.AreEqual(c, result[0]);
            Assert.AreEqual(1, query.TestCasesStartedCount);
        }

        private static string RandomId() => Guid.NewGuid().ToString();
    }
}
