using Io.Cucumber.Messages.Types;
using System;
using System.Collections.Generic;

namespace Io.Cucumber.Query
{
    internal static class Lineages
    {
        /// <summary>
        /// Create map of a GherkinDocument element to its Lineage in that document.
        /// </summary>
        /// <param name="document">The GherkinDocument to create the lineage of.</param>
        /// <returns>A map of the document elements to their lineage.</returns>
        public static Dictionary<string, Lineage> Of(GherkinDocument document)
        {
            var elements = new Dictionary<string, Lineage>();
            var lineage = new Lineage(document);
            var uri = document.Uri ?? throw new ArgumentException("document.uri must not be null");
            elements[uri] = lineage;
            if (document.Feature != null)
                OfFeature(lineage, elements)(document.Feature);
            return elements;
        }

        private static Action<Feature> OfFeature(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return feature =>
            {
                var lineage = new Lineage(parent, feature);
                foreach (var child in feature.Children)
                    OfFeatureChild(lineage, elements)(child);
            };
        }

        private static Action<FeatureChild> OfFeatureChild(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return featureChild =>
            {
                if (featureChild.Scenario != null)
                    OfScenario(parent, elements)(featureChild.Scenario);
                if (featureChild.Rule != null)
                    OfRule(parent, elements)(featureChild.Rule);
            };
        }

        private static Action<Rule> OfRule(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return rule =>
            {
                var lineage = new Lineage(parent, rule);
                elements[rule.Id] = lineage;
                foreach (var child in rule.Children)
                    OfRuleChild(lineage, elements)(child);
            };
        }

        private static Action<RuleChild> OfRuleChild(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return ruleChild =>
            {
                if (ruleChild.Scenario != null)
                    OfScenario(parent, elements)(ruleChild.Scenario);
            };
        }

        private static Action<Scenario> OfScenario(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return scenario =>
            {
                var lineage = new Lineage(parent, scenario);
                elements[scenario.Id] = lineage;
                ForEachIndexed(scenario.Examples, OfExamples(lineage, elements));
            };
        }

        private static Action<Examples, int> OfExamples(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return (examples, examplesIndex) =>
            {
                var lineage = new Lineage(parent, examples, examplesIndex);
                elements[examples.Id] = lineage;
                ForEachIndexed(examples.TableBody, OfExample(lineage, elements));
            };
        }

        private static Action<TableRow, int> OfExample(Lineage parent, Dictionary<string, Lineage> elements)
        {
            return (example, exampleIndex) =>
            {
                var lineage = new Lineage(parent, example, exampleIndex);
                elements[example.Id] = lineage;
            };
        }

        private static void ForEachIndexed<T>(IList<T> items, Action<T, int> consumer)
        {
            if (items == null) return;
            for (int i = 0; i < items.Count; i++)
            {
                consumer(items[i], i);
            }
        }
    }
}
