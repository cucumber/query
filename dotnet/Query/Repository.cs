#nullable enable
using Io.Cucumber.Messages.Types;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System;
using System.Linq;

namespace Io.Cucumber.Query
{
    public class Repository
    {
        // Features
        public enum RepositoryFeature
        {
            /// <summary>
            /// Include <see cref="Attachment"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_ATTACHMENTS,

            /// <summary>
            /// Include <see cref="Io.Cucumber.Messages.Types.GherkinDocument"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_GHERKIN_DOCUMENTS,

            /// <summary>
            /// Include <see cref="Hook"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_HOOKS,

            /// <summary>
            /// Include <see cref="StepDefinition"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_STEP_DEFINITIONS,

            /// <summary>
            /// Include <see cref="Suggestion"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_SUGGESTIONS,

            /// <summary>
            /// Include <see cref="UndefinedParameterType"/> messages.
            /// Disable to reduce memory usage.
            /// </summary>
            INCLUDE_UNDEFINED_PARAMETER_TYPES,
        }

        private readonly HashSet<RepositoryFeature> _features;

        // Public fields (matching Java order)
        public readonly Dictionary<string, TestCaseStarted> TestCaseStartedById = new();
        public readonly Dictionary<string, TestCaseFinished> TestCaseFinishedByTestCaseStartedId = new();
        public readonly Dictionary<string, List<TestStepFinished>> TestStepsFinishedByTestCaseStartedId = new();
        public readonly Dictionary<string, List<TestStepStarted>> TestStepsStartedByTestCaseStartedId = new();
        public readonly Dictionary<string, TestRunHookStarted> TestRunHookStartedById = new();
        public readonly Dictionary<string, TestRunHookFinished> TestRunHookFinishedByTestRunHookStartedId = new();
        public readonly Dictionary<string, Pickle> PickleById = new();
        public readonly Dictionary<string, TestCase> TestCaseById = new();
        public readonly Dictionary<string, Step> StepById = new();
        public readonly Dictionary<string, TestStep> TestStepById = new();
        public readonly Dictionary<string, PickleStep> PickleStepById = new();
        public readonly Dictionary<string, Hook> HookById = new();
        public readonly Dictionary<string, List<Attachment>> AttachmentsByTestCaseStartedId = new();
        public readonly Dictionary<string, List<Attachment>> AttachmentsByTestRunHookStartedId = new();
        public readonly Dictionary<object, Lineage> LineageById = new();
        public readonly Dictionary<string, StepDefinition> StepDefinitionById = new();
        public readonly Dictionary<string, List<Suggestion>> SuggestionsByPickleStepId = new();
        public readonly List<UndefinedParameterType> UndefinedParameterTypes = new();

        public Meta? Meta;
        public TestRunStarted? TestRunStarted;
        public TestRunFinished? TestRunFinished;

        public static Repository CreateWithAllFeatures()
        {
            return new Repository((RepositoryFeature[])Enum.GetValues(typeof(RepositoryFeature)));
        }

        public Repository(IEnumerable<RepositoryFeature>? features = null)
        {
            _features = features != null
                ? new HashSet<RepositoryFeature>(features)
                : new HashSet<RepositoryFeature>();
        }

        public void Update(Envelope envelope)
        {
            if (envelope.Meta != null) UpdateMeta(envelope.Meta);
            if (envelope.TestRunStarted != null) UpdateTestRunStarted(envelope.TestRunStarted);
            if (envelope.TestRunFinished != null) UpdateTestRunFinished(envelope.TestRunFinished);
            if (envelope.TestRunHookStarted != null) UpdateTestRunHookStarted(envelope.TestRunHookStarted);
            if (envelope.TestRunHookFinished != null) UpdateTestRunHookFinished(envelope.TestRunHookFinished);
            if (envelope.TestCaseStarted != null) UpdateTestCaseStarted(envelope.TestCaseStarted);
            if (envelope.TestCaseFinished != null) UpdateTestCaseFinished(envelope.TestCaseFinished);
            if (envelope.TestStepStarted != null) UpdateTestStepStarted(envelope.TestStepStarted);
            if (envelope.TestStepFinished != null) UpdateTestStepFinished(envelope.TestStepFinished);
            if (envelope.Pickle != null) UpdatePickle(envelope.Pickle);
            if (envelope.TestCase != null) UpdateTestCase(envelope.TestCase);
            if (_features.Contains(RepositoryFeature.INCLUDE_GHERKIN_DOCUMENTS) && envelope.GherkinDocument != null) UpdateGherkinDocument(envelope.GherkinDocument);
            if (_features.Contains(RepositoryFeature.INCLUDE_STEP_DEFINITIONS) && envelope.StepDefinition != null) UpdateStepDefinition(envelope.StepDefinition);
            if (_features.Contains(RepositoryFeature.INCLUDE_HOOKS) && envelope.Hook != null) UpdateHook(envelope.Hook);
            if (_features.Contains(RepositoryFeature.INCLUDE_ATTACHMENTS) && envelope.Attachment != null) UpdateAttachment(envelope.Attachment);
            if (_features.Contains(RepositoryFeature.INCLUDE_SUGGESTIONS) && envelope.Suggestion != null) UpdateSuggestions(envelope.Suggestion);
            if (_features.Contains(RepositoryFeature.INCLUDE_UNDEFINED_PARAMETER_TYPES) && envelope.UndefinedParameterType != null) UpdateUndefinedParameterType(envelope.UndefinedParameterType);
        }

        internal void UpdateAttachment(Attachment attachment)
        {
            if (attachment.TestCaseStartedId != null)
            {
                if (!AttachmentsByTestCaseStartedId.TryGetValue(attachment.TestCaseStartedId, out var list))
                {
                    list = new List<Attachment>();
                    AttachmentsByTestCaseStartedId[attachment.TestCaseStartedId] = list;
                }
                list.Add(attachment);
            }
            if (attachment.TestRunHookStartedId != null)
            {
                if (!AttachmentsByTestRunHookStartedId.TryGetValue(attachment.TestRunHookStartedId, out var list))
                {
                    list = new List<Attachment>();
                    AttachmentsByTestRunHookStartedId[attachment.TestRunHookStartedId] = list;
                }
                list.Add(attachment);
            }
        }

        internal void UpdateHook(Hook hook)
        {
            HookById[hook.Id] = hook;
        }

        internal void UpdateTestCaseStarted(TestCaseStarted testCaseStarted)
        {
            TestCaseStartedById[testCaseStarted.Id] = testCaseStarted;
        }

        internal void UpdateTestCase(TestCase testCase)
        {
            TestCaseById[testCase.Id] = testCase;
            foreach (var testStep in testCase.TestSteps)
            {
                TestStepById[testStep.Id] = testStep;
            }
        }

        internal void UpdatePickle(Pickle pickle)
        {
            PickleById[pickle.Id] = pickle;
            foreach (var step in pickle.Steps)
            {
                PickleStepById[step.Id] = step;
            }
        }

        internal void UpdateGherkinDocument(GherkinDocument document)
        {
            foreach (var lineage in Lineages.Of(document))
            {
                LineageById.Add(lineage.Key, lineage.Value);
            }
            if (document.Feature != null)
            {
                UpdateFeature(document.Feature);
            }
        }

        internal void UpdateFeature(Feature feature)
        {
            foreach (var child in feature.Children)
            {
                if (child.Background != null)
                {
                    UpdateSteps(child.Background.Steps);
                }
                if (child.Scenario != null)
                {
                    UpdateScenario(child.Scenario);
                }
                if (child.Rule != null)
                {
                    foreach (var ruleChild in child.Rule.Children)
                    {
                        if (ruleChild.Background != null)
                        {
                            UpdateSteps(ruleChild.Background.Steps);
                        }
                        if (ruleChild.Scenario != null)
                        {
                            UpdateScenario(ruleChild.Scenario);
                        }
                    }
                }
            }
        }

        internal void UpdateTestStepStarted(TestStepStarted testStepStarted)
        {
            if (!TestStepsStartedByTestCaseStartedId.TryGetValue(testStepStarted.TestCaseStartedId, out var list))
            {
                list = new List<TestStepStarted>();
                TestStepsStartedByTestCaseStartedId[testStepStarted.TestCaseStartedId] = list;
            }
            list.Add(testStepStarted);
        }

        internal void UpdateTestStepFinished(TestStepFinished testStepFinished)
        {
            if (!TestStepsFinishedByTestCaseStartedId.TryGetValue(testStepFinished.TestCaseStartedId, out var list))
            {
                list = new List<TestStepFinished>();
                TestStepsFinishedByTestCaseStartedId[testStepFinished.TestCaseStartedId] = list;
            }
            list.Add(testStepFinished);
        }

        internal void UpdateTestCaseFinished(TestCaseFinished testCaseFinished)
        {
            TestCaseFinishedByTestCaseStartedId[testCaseFinished.TestCaseStartedId] = testCaseFinished;
        }

        internal void UpdateTestRunFinished(TestRunFinished testRunFinished)
        {
            TestRunFinished = testRunFinished;
        }

        internal void UpdateTestRunStarted(TestRunStarted testRunStarted)
        {
            TestRunStarted = testRunStarted;
        }

        internal void UpdateTestRunHookStarted(TestRunHookStarted testRunHookStarted)
        {
            TestRunHookStartedById[testRunHookStarted.Id] = testRunHookStarted;
        }

        internal void UpdateTestRunHookFinished(TestRunHookFinished testRunHookFinished)
        {
            TestRunHookFinishedByTestRunHookStartedId[testRunHookFinished.TestRunHookStartedId] = testRunHookFinished;
        }

        internal void UpdateScenario(Scenario scenario)
        {
            UpdateSteps(scenario.Steps);
        }

        internal void UpdateStepDefinition(StepDefinition stepDefinition)
        {
            StepDefinitionById[stepDefinition.Id] = stepDefinition;
        }

        internal void UpdateSteps(IList<Step> steps)
        {
            foreach (var step in steps)
            {
                StepById[step.Id] = step;
            }
        }

        internal void UpdateSuggestions(Suggestion suggestion)
        {
            if (!SuggestionsByPickleStepId.TryGetValue(suggestion.PickleStepId, out var list))
            {
                list = new List<Suggestion>();
                SuggestionsByPickleStepId[suggestion.PickleStepId] = list;
            }
            list.Add(suggestion);
        }

        internal void UpdateMeta(Meta meta)
        {
            Meta = meta;
        }

        internal void UpdateUndefinedParameterType(UndefinedParameterType undefinedParameterType)
        {
            UndefinedParameterTypes.Add(undefinedParameterType);
        }
    }
}
