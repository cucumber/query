using System;
using Io.Cucumber.Messages.Types;

namespace Io.Cucumber.Query
{
    // port of io.cucumber.query.LineageReducer<T> (Java)
    public interface ILineageReducer<T>
    {
        T Reduce(Lineage lineage);
        T Reduce(Lineage lineage, Pickle pickle);
    }

    // port of io.cucumber.query.LineageReducer.Collector<T> (Java)
    public interface ICollector<T>
    {
        void Add(GherkinDocument document);
        void Add(Feature feature);
        void Add(Rule rule);
        void Add(Scenario scenario);
        void Add(Examples examples, int index);
        void Add(TableRow example, int index);
        void Add(Pickle pickle);
        T Finish();
    }
}
