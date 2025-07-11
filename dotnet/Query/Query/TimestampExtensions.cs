using Io.Cucumber.Messages.Types;
using System;

namespace Io.Cucumber.Query
{
    internal static class TimestampExtensions
    {

        internal static DateTimeOffset ToDateTimeOffset(this Timestamp timestamp)
        {
            // Java: Convertor.toInstant(timestamp)
            // C#: Timestamp has Seconds and Nanos
            var dateTime = DateTimeOffset.FromUnixTimeSeconds(timestamp.Seconds);
            return dateTime.AddTicks(timestamp.Nanos / 100);
        }
    }
}