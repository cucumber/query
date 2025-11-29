using Io.Cucumber.Messages.Types;
using Cucumber.Query;

namespace Cucumber.QueryTest
{
    [TestClass]
    public class TimestampComparerTest
    {
        private readonly TimestampComparer comparer = new TimestampComparer();

        [TestMethod]
        public void Identity()
        {
            var a = new Timestamp(1L, 1L);
            var b = new Timestamp(1L, 1L);

            Assert.AreEqual(0, comparer.Compare(a, b));
            Assert.AreEqual(0, comparer.Compare(b, a));
        }

        [TestMethod]
        public void OnSeconds()
        {
            var a = new Timestamp(1L, 1L);
            var b = new Timestamp(2L, 2L);
            Assert.AreEqual(-1, comparer.Compare(a, b));
            Assert.AreEqual(1, comparer.Compare(b, a));
        }

        [TestMethod]
        public void OnNanoSeconds()
        {
            var a = new Timestamp(1L, 1L);
            var b1 = new Timestamp(1L, 0L);
            var b2 = new Timestamp(1L, 2L);

            Assert.AreEqual(1, comparer.Compare(a, b1));
            Assert.AreEqual(-1, comparer.Compare(b1, a));

            Assert.AreEqual(-1, comparer.Compare(a, b2));
            Assert.AreEqual(1, comparer.Compare(b2, a));
        }
    }
}
