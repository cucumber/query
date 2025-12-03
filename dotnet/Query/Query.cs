#nullable enable

using Cucumber.Messages;
using Io.Cucumber.Messages.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Cucumber.Query;

public class Query
{
    private readonly Repository _repository;

    public Query(Repository repository)
    {
        _repository = repository;
    }

    public IDictionary<TestStepResultStatus, long> CountMostSevereTestStepResultStatus()
    {
        var statusCounts = new Dictionary<TestStepResultStatus, long>();
        // Initialize with zero for each possible TestStepResultStatus
        foreach (TestStepResultStatus status in Enum.GetValues(typeof(TestStepResultStatus)))
        {
            statusCounts[status] = 0;
        }
        foreach (var testCaseStarted in FindAllTestCaseStarted())
        {
            var finishedSteps = FindTestStepsFinishedBy(testCaseStarted).ToList();
            if (finishedSteps.Count == 0)
                continue;
            // Find the most severe status (largest enum value)
            var mostSevere = finishedSteps
                .Select(f => f.TestStepResult.Status)
                .Max();
            if (statusCounts.ContainsKey(mostSevere))
                statusCounts[mostSevere]++;
            else
                statusCounts[mostSevere] = 1;
        }
        return statusCounts;
    }

    public int TestCasesStartedCount => FindAllTestCaseStarted().Count();

    public IEnumerable<Pickle> FindAllPickles() => _repository.PickleById.Values;

    public IEnumerable<PickleStep> FindAllPickleSteps() => _repository.PickleStepById.Values;

    public IEnumerable<TestCaseStarted> FindAllTestCaseStarted() => _repository.TestCaseStartedById.Values
        .Where(tcs => !FindTestCaseFinishedBy(tcs)?.WillBeRetried ?? true); // Exclude finished cases that will be retried

    public IEnumerable<StepDefinition> FindAllStepDefinitions() => _repository.StepDefinitionById.Values;

    public IEnumerable<TestCaseFinished> FindAllTestCaseFinished()
    {
        return _repository.TestCaseFinishedByTestCaseStartedId.Values
            .Where(tcFinished => !tcFinished.WillBeRetried);
    }

    public IEnumerable<TestStep> FindAllTestSteps() => _repository.TestStepById.Values;

    public IEnumerable<TestCase> FindAllTestCases()
    {
        return _repository.TestCaseById.Values;
    }

    public IEnumerable<TestStepStarted> FindAllTestStepStarted()
    {
        return _repository.TestStepsStartedByTestCaseStartedId.Values
            .SelectMany(list => list);
    }

    public IEnumerable<TestStepFinished> FindAllTestStepFinished()
    {
        return _repository.TestStepsFinishedByTestCaseStartedId.Values
            .SelectMany(list => list);
    }

    public IEnumerable<TestRunHookStarted> FindAllTestRunHookStarted()
    {
        return _repository.TestRunHookStartedById.Values;
    }

    public IEnumerable<TestRunHookFinished> FindAllTestRunHookFinished()
    {
        return _repository.TestRunHookFinishedByTestRunHookStartedId.Values;
    }

    public IEnumerable<UndefinedParameterType> FindAllUndefinedParameterTypes()
    {
        return _repository.UndefinedParameterTypes;
    }

    public IEnumerable<Attachment> FindAttachmentsBy(TestStepFinished testStepFinished) =>
        _repository.AttachmentsByTestCaseStartedId.TryGetValue(testStepFinished.TestCaseStartedId, out var attachments)
            ? attachments.Where(a => a.TestStepId == testStepFinished.TestStepId)
            : new List<Attachment>();

    public IEnumerable<Attachment> FindAttachmentsBy(TestRunHookFinished testRunHookFinished)
    {
        if (_repository.AttachmentsByTestRunHookStartedId.TryGetValue(testRunHookFinished.TestRunHookStartedId, out var attachments))
            return new List<Attachment>(attachments);
        return new List<Attachment>();
    }

    public Hook? FindHookBy(TestStep testStep)
    {
        if (!string.IsNullOrEmpty(testStep.HookId) && _repository.HookById.TryGetValue(testStep.HookId, out var hook))
        {
            return hook;
        }
        return null;
    }

    public Hook? FindHookBy(TestRunHookStarted testRunHookStarted)
    {
        if (!string.IsNullOrEmpty(testRunHookStarted.HookId) && _repository.HookById.TryGetValue(testRunHookStarted.HookId, out var hook))
        {
            return hook;
        }
        return null;
    }

    public Hook? FindHookBy(TestRunHookFinished testRunHookFinished)
    {
        var testRunHookStarted = FindTestRunHookStartedBy(testRunHookFinished);
        if (testRunHookStarted == null)
        {
            return null;
        }
        return FindHookBy(testRunHookStarted);
    }

    public Meta? FindMeta() => _repository.Meta;

    public TestStepResult? FindMostSevereTestStepResultBy(TestCaseStarted testCaseStarted)
    {
        var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
        if (finishedSteps.Count() == 0)
            return null;
        // Find the TestStepFinished with the most severe status (highest enum value)
        var mostSevere = finishedSteps
            .OrderBy(f => f.TestStepResult.Status)
            .LastOrDefault();
        return mostSevere?.TestStepResult;
    }

    public TestStepResult? FindMostSevereTestStepResultBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindMostSevereTestStepResultBy(testCaseStarted) : null;
    }

    public Location? FindLocationOf(Pickle pickle)
    {
        var lineage = FindLineageBy(pickle);
        if (lineage == null)
            return null;
        if (lineage.Example != null)
            return lineage.Example.Location;
        if (lineage.Scenario != null)
            return lineage.Scenario.Location;
        return null;
    }

    public Pickle? FindPickleBy(TestCaseStarted testCaseStarted)
    {
        var testCase = FindTestCaseBy(testCaseStarted);
        if (testCase != null && _repository.PickleById.TryGetValue(testCase.PickleId, out var pickle))
        {
            return pickle;
        }
        return null;
    }

    public Pickle? FindPickleBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    public Pickle? FindPickleBy(TestCase testCase)
    {
        return testCase != null && _repository.PickleById.TryGetValue(testCase.PickleId, out var pickle) ? pickle : null;
    }

    public Pickle? FindPickleBy(TestStepStarted testStepStarted)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    public Pickle? FindPickleBy(TestStepFinished testStepFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepFinished);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    public PickleStep? FindPickleStepBy(TestStep testStep)
    {
        if (!string.IsNullOrEmpty(testStep.PickleStepId))
        {
            if (_repository.PickleStepById.TryGetValue(testStep.PickleStepId, out var pickleStep))
            {
                return pickleStep;
            }
        }
        return null;
    }

    public IEnumerable<Suggestion> FindSuggestionsBy(PickleStep pickleStep)
    {
        if (_repository.SuggestionsByPickleStepId.TryGetValue(pickleStep.Id, out var suggestions))
            return new List<Suggestion>(suggestions);
        return new List<Suggestion>();
    }

    public IEnumerable<Suggestion> FindSuggestionsBy(Pickle pickle)
    {
        var result = new List<Suggestion>();
        foreach (var step in pickle.Steps)
            result.AddRange(FindSuggestionsBy(step));
        return result;
    }

    public Step? FindStepBy(PickleStep pickleStep)
    {
        if (pickleStep.AstNodeIds != null && pickleStep.AstNodeIds.Count > 0)
        {
            var stepId = pickleStep.AstNodeIds[0];
            if (!string.IsNullOrEmpty(stepId) && _repository.StepById.TryGetValue(stepId, out var step))
            {
                return step;
            }
        }
        return null;
    }

    public IEnumerable<StepDefinition> FindStepDefinitionsBy(TestStep testStep)
    {
        if (testStep.StepDefinitionIds != null)
            return testStep.StepDefinitionIds
                .Select(id => _repository.StepDefinitionById.TryGetValue(id, out var def) ? def : null)
                .Where(def => def != null)
                .Cast<StepDefinition>();
        return new List<StepDefinition>();
    }

    public StepDefinition? FindUnambiguousStepDefinitionBy(TestStep testStep)
    {
        if (testStep.StepDefinitionIds != null && testStep.StepDefinitionIds.Count == 1)
            return _repository.StepDefinitionById.TryGetValue(testStep.StepDefinitionIds[0], out var def) ? def : null;
        return null;
    }

    public TestCase? FindTestCaseBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestCaseById.TryGetValue(testCaseStarted.TestCaseId, out var testCase))
        {
            return testCase;
        }
        return null;
    }

    public TestCase? FindTestCaseBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    public TestCase? FindTestCaseBy(TestStepStarted testStepStarted)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    public TestCase? FindTestCaseBy(TestStepFinished testStepFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepFinished);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    public System.TimeSpan? FindTestCaseDurationBy(TestCaseStarted testCaseStarted)
    {
        var started = testCaseStarted.Timestamp;
        var finished = FindTestCaseFinishedBy(testCaseStarted)?.Timestamp;
        if (finished != null)
        {
            var startTime = Converters.ToDateTime(started);
            var finishTime = Converters.ToDateTime(finished);
            return finishTime - startTime;
        }
        return null;
    }

    public System.TimeSpan? FindTestCaseDurationBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestCaseDurationBy(testCaseStarted) : null;
    }

    public TestCaseStarted? FindTestCaseStartedBy(TestStepStarted testStepStarted)
    {
        return _repository.TestCaseStartedById.TryGetValue(testStepStarted.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    public TestCaseStarted? FindTestCaseStartedBy(TestCaseFinished testCaseFinished)
    {
        return _repository.TestCaseStartedById.TryGetValue(testCaseFinished.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    public TestCaseStarted? FindTestCaseStartedBy(TestStepFinished testStepFinished)
    {
        return _repository.TestCaseStartedById.TryGetValue(testStepFinished.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    public TestCaseFinished? FindTestCaseFinishedBy(TestCaseStarted testCaseStarted)
    {
        return _repository.TestCaseFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var finished) ? finished : null;
    }

    public TestRunHookFinished? FindTestRunHookFinishedBy(TestRunHookStarted testRunHookStarted)
    {
        return _repository.TestRunHookFinishedByTestRunHookStartedId.TryGetValue(testRunHookStarted.Id, out var finished) ? finished : null;
    }

    public TestRunHookStarted? FindTestRunHookStartedBy(TestRunHookFinished testRunHookFinished)
    {
        return _repository.TestRunHookStartedById.TryGetValue(testRunHookFinished.TestRunHookStartedId, out var started) ? started : null;
    }

    public System.TimeSpan? FindTestRunDuration()
    {
        if (_repository.TestRunStarted == null || _repository.TestRunFinished == null)
            return null;
        var start = Converters.ToDateTime(_repository.TestRunStarted.Timestamp);
        var finish = Converters.ToDateTime(_repository.TestRunFinished.Timestamp);
        return finish - start;
    }

    public TestRunFinished? FindTestRunFinished() => _repository.TestRunFinished;

    public TestRunStarted? FindTestRunStarted() => _repository.TestRunStarted;

    public TestStep? FindTestStepBy(TestStepStarted testStepStarted)
    {
        return _repository.TestStepById.TryGetValue(testStepStarted.TestStepId, out var testStep) ? testStep : null;
    }

    public TestStep? FindTestStepBy(TestStepFinished testStepFinished)
    {
        return _repository.TestStepById.TryGetValue(testStepFinished.TestStepId, out var testStep) ? testStep : null;
    }

    public IEnumerable<TestStepStarted> FindTestStepsStartedBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestStepsStartedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            return new List<TestStepStarted>(steps);
        }
        return new List<TestStepStarted>();
    }

    public IEnumerable<TestStepStarted> FindTestStepsStartedBy(TestCaseFinished testCaseFinished)
    {
        if (_repository.TestStepsStartedByTestCaseStartedId.TryGetValue(testCaseFinished.TestCaseStartedId, out var steps))
        {
            return new List<TestStepStarted>(steps);
        }
        return new List<TestStepStarted>();
    }

    public IEnumerable<TestStepFinished> FindTestStepsFinishedBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestStepsFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            return new List<TestStepFinished>(steps);
        }
        return new List<TestStepFinished>();
    }

    public IEnumerable<TestStepFinished> FindTestStepsFinishedBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestStepsFinishedBy(testCaseStarted) : new List<TestStepFinished>();
    }

    public IEnumerable<(TestStepFinished, TestStep)> FindTestStepFinishedAndTestStepBy(TestCaseStarted testCaseStarted)
    {
        var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
        var result = new List<(TestStepFinished, TestStep)>();
        foreach (var testStepFinished in finishedSteps)
        {
            var testStep = FindTestStepBy(testStepFinished);
            if (testStep != null)
            {
                result.Add((testStepFinished, testStep));
            }
        }
        return result;
    }

    // FindLineageBy methods
    public Lineage? FindLineageBy(GherkinDocument element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Feature element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Rule element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Scenario element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Examples element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(TableRow element)
    {
        _repository.LineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Pickle pickle)
    {
        var astNodeIds = pickle.AstNodeIds;
        var lastAstNodeId = astNodeIds.LastOrDefault();
        _repository.LineageById.TryGetValue(lastAstNodeId, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(TestCaseStarted testCaseStarted)
    {
        var pickle = FindPickleBy(testCaseStarted);
        if (pickle == null)
        {
            return null;
        }
        return FindLineageBy(pickle);
    }

    public Lineage? FindLineageBy(TestCaseFinished testCaseFinished)
    {
        var pickle = FindPickleBy(testCaseFinished);
        if (pickle == null)
        {
            return null;
        }
        return FindLineageBy(pickle);
    }
}
