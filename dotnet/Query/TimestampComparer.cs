using Io.Cucumber.Messages.Types;
using System.Collections.Generic;

namespace Io.Cucumber.Query
{
    internal class TimestampComparer : IComparer<Timestamp>
    {
        public int Compare(Timestamp a, Timestamp b)
        {
            long sa = a.Seconds;
            long sb = b.Seconds;

            if (sa < sb)
                return -1;
            else if (sb < sa)
                return 1;

            long na = a.Nanos;
            long nb = b.Nanos;

            if (na < nb)
                return -1;
            else if (nb < na)
                return 1;

            return 0;
        }
    }
}