using System;
using System.Collections.Generic;
using System.Linq;
using Io.Cucumber.Messages.Types;

namespace Io.Cucumber.Query
{
    /// <summary>
    /// Names GherkinDocument elements or Pickles.
    /// </summary>
    internal class NamingCollector : ICollector<string>
    {
        // There are at most 5 levels to a feature file.
        private readonly List<string> parts = new List<string>(5);
        private readonly string delimiter = " - ";
        private readonly Strategy strategy;
        private readonly FeatureName featureName;
        private readonly ExampleName exampleName;

        private string scenarioName;
        private bool isExample;
        private int examplesIndex;

        public static Func<NamingCollector> Of(Strategy strategy, FeatureName featureName, ExampleName exampleName)
        {
            return () => new NamingCollector(strategy, featureName, exampleName);
        }

        public NamingCollector(Strategy strategy, FeatureName featureName, ExampleName exampleName)
        {
            this.strategy = strategy;
            this.featureName = featureName;
            this.exampleName = exampleName;
        }

        public void Add(GherkinDocument document) { }
        public void Add(Feature feature)
        {
            if (featureName == FeatureName.INCLUDE || strategy == Strategy.SHORT)
            {
                parts.Add(feature.Name);
            }
        }
        public void Add(Rule rule)
        {
            parts.Add(rule.Name);
        }
        public void Add(Scenario scenario)
        {
            scenarioName = scenario.Name;
            parts.Add(scenarioName);
        }
        public void Add(Examples examples, int index)
        {
            parts.Add(examples.Name);
            this.examplesIndex = index;
        }
        public void Add(TableRow example, int index)
        {
            isExample = true;
            parts.Add("#" + (examplesIndex + 1) + "." + (index + 1));
        }
        public void Add(Pickle pickle)
        {
            string pickleName = pickle.Name;

            // Case 0: Pickles with an empty lineage
            if (scenarioName == null)
            {
                parts.Add(pickleName);
                return;
            }

            // Case 1: Pickles from a scenario outline
            if (isExample)
            {
                switch (exampleName)
                {
                    case ExampleName.NUMBER:
                        break;
                    case ExampleName.NUMBER_AND_PICKLE_IF_PARAMETERIZED:
                        bool parameterized = !scenarioName.Equals(pickleName);
                        if (parameterized)
                        {
                            string exampleNumber = parts[parts.Count - 1];
                            parts.RemoveAt(parts.Count - 1);
                            parts.Add(exampleNumber + ": " + pickleName);
                        }
                        break;
                    case ExampleName.PICKLE:
                        parts.RemoveAt(parts.Count - 1); // Remove example number
                        parts.Add(pickleName);
                        break;
                }
            }
            // Case 2: Pickles from a scenario
            // Nothing to do, scenario name and pickle name are the same.
        }

        public string Finish()
        {
            if (strategy == Strategy.SHORT)
            {
                return parts.Count > 0 ? parts[parts.Count - 1] : string.Empty;
            }
            return string.Join(delimiter, parts.Where(s => !string.IsNullOrEmpty(s)));
        }
    }
}