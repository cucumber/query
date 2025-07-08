using System;
using Io.Cucumber.Messages.Types;

namespace Io.Cucumber.Query
{
    /// <summary>
    /// Names Pickles and other elements in a GherkinDocument.
    /// </summary>
    public enum Strategy
    {
        LONG,
        SHORT
    }

    public enum ExampleName
    {
        NUMBER,
        PICKLE,
        NUMBER_AND_PICKLE_IF_PARAMETERIZED
    }

    public enum FeatureName
    {
        INCLUDE,
        EXCLUDE
    }
    public abstract class NamingStrategy : ILineageReducer<string>
    {

        public static Builder Create(Strategy strategy)
        {
            return new Builder(strategy);
        }

        protected NamingStrategy() { }

        public abstract string Reduce(Lineage lineage);
        public abstract string Reduce(Lineage lineage, Pickle pickle);

        public class Builder
        {
            private readonly Strategy _strategy;
            private FeatureName _featureName = Io.Cucumber.Query.FeatureName.INCLUDE;
            private ExampleName _exampleName = Io.Cucumber.Query.ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED;

            public Builder(Strategy strategy)
            {
                _strategy = strategy;
            }

            public Builder ExampleName(ExampleName exampleName)
            {
                _exampleName = exampleName;
                return this;
            }

            public Builder FeatureName(FeatureName featureName)
            {
                _featureName = featureName;
                return this;
            }

            public NamingStrategy Build()
            {
                return new Adaptor(
                    new LineageReducerDescending<string>(
                        () => new NamingCollector((Strategy)_strategy, (FeatureName)_featureName, (ExampleName)_exampleName)
                    )
                );
            }
        }

        private class Adaptor : NamingStrategy
        {
            private readonly ILineageReducer<string> _delegate;

            public Adaptor(ILineageReducer<string> del)
            {
                _delegate = del;
            }

            public override string Reduce(Lineage lineage)
            {
                return _delegate.Reduce(lineage);
            }

            public override string Reduce(Lineage lineage, Pickle pickle)
            {
                return _delegate.Reduce(lineage, pickle);
            }
        }
    }
}
