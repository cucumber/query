using System;
using Io.Cucumber.Messages.Types;

namespace Io.Cucumber.Query
{
    // port of io.cucumber.query.LineageReducerDescending (Java)
    public class LineageReducerDescending<T> : ILineageReducer<T>
    {
        private readonly Func<ICollector<T>> _collectorSupplier;

        public LineageReducerDescending(Func<ICollector<T>> collectorSupplier)
        {
            _collectorSupplier = collectorSupplier ?? throw new ArgumentNullException(nameof(collectorSupplier));
        }

        public T Reduce(Lineage lineage)
        {
            var collector = _collectorSupplier();
            ReduceAddLineage(collector, lineage);
            return collector.Finish();
        }

        public T Reduce(Lineage lineage, Pickle pickle)
        {
            var collector = _collectorSupplier();
            ReduceAddLineage(collector, lineage);
            collector.Add(pickle);
            return collector.Finish();
        }

        private static void ReduceAddLineage(ICollector<T> collector, Lineage lineage)
        {
            if (lineage.Document != null)
                collector.Add(lineage.Document);
            if (lineage.Feature != null)
                collector.Add(lineage.Feature);
            if (lineage.Rule != null)
                collector.Add(lineage.Rule);
            if (lineage.Scenario != null)
                collector.Add(lineage.Scenario);
            if (lineage.Examples != null)
                collector.Add(lineage.Examples, lineage.ExamplesIndex ?? 0);
            if (lineage.Example != null)
                collector.Add(lineage.Example, lineage.ExampleIndex ?? 0);
        }
    }
}
