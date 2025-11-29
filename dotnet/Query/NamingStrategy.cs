using Io.Cucumber.Messages.Types;

namespace Cucumber.Query
{
    /// <summary>
    /// Names Pickles and other elements in a GherkinDocument.
    /// </summary>
    public abstract class NamingStrategy : ILineageReducer<string>
    {
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

        public static NamingStrategy Create(Strategy strategy, FeatureName featureName = FeatureName.INCLUDE, ExampleName exampleName = ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED)
        {
            return new Adaptor(
                    new LineageReducerDescending<string>(
                        () => new NamingCollector(strategy, featureName, exampleName)
                    ));
        }

        public static NamingStrategy Create(Strategy strategy, ExampleName exampleName)
        {
            return Create(strategy, FeatureName.INCLUDE, exampleName);
        }

        public abstract string Reduce(Lineage lineage);

        public abstract string Reduce(Lineage lineage, Pickle pickle);

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
