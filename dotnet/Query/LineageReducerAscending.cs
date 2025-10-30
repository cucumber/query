using System;
using Io.Cucumber.Messages.Types;

namespace Io.Cucumber.Query
{
    public class LineageReducerAscending<T> : ILineageReducer<T>
    {
        private readonly Func<ICollector<T>> _collectorSupplier;

        public LineageReducerAscending(Func<ICollector<T>> collectorSupplier)
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
            collector.Add(pickle);
            ReduceAddLineage(collector, lineage);
            return collector.Finish();
        }

        private static void ReduceAddLineage(ICollector<T> collector, Lineage lineage)
        {
            if (lineage.Example != null)
                collector.Add(lineage.Example, lineage.ExampleIndex ?? 0);
            if (lineage.Examples != null)
                collector.Add(lineage.Examples, lineage.ExamplesIndex ?? 0);
            if (lineage.Scenario != null)
                collector.Add(lineage.Scenario);
            if (lineage.Rule != null)
                collector.Add(lineage.Rule);
            if (lineage.Feature != null)
                collector.Add(lineage.Feature);
            if (lineage.Document != null)
                collector.Add(lineage.Document);
        }
    }
}
