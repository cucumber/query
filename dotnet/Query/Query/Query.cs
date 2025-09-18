#nullable enable
using System;
using System.ComponentModel.DataAnnotations;
using System.Collections.Generic;
using System.Linq;
using Io.Cucumber.Messages.Types;
using System.Collections.Concurrent;
using Cucumber.Messages;

namespace Io.Cucumber.Query;

// Ported from io.cucumber.query.Query (Java)
public class Query
{
    private readonly Repository _repository;

    public Query(Repository repository)
    {
        _repository = repository;
    }

    // 1. countMostSevereTestStepResultStatus
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
            var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
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

    // 2. countTestCasesStarted
    public int TestCasesStartedCount => FindAllTestCaseStarted().Count;

    // 3. FindAllPickles
    public IList<Pickle> FindAllPickles() => _repository.PickleById.Values.ToList();

    // 4. FindAllPickleSteps
    public IList<PickleStep> FindAllPickleSteps() => _repository.PickleStepById.Values.ToList();

    // 5. FindAllTestCaseStarted
    public IList<TestCaseStarted> FindAllTestCaseStarted() => _repository.TestCaseStartedById.Values
        .Where(tcs => !FindTestCaseFinishedBy(tcs)?.WillBeRetried ?? true) // Exclude finished cases that will be retried
        .ToList();

    // 6. FindAllTestCaseFinished
    public IList<TestCaseFinished> FindAllTestCaseFinished()
    {
        return _repository.TestCaseFinishedByTestCaseStartedId.Values
            .Where(tcFinished => !tcFinished.WillBeRetried)
            .ToList();
    }

    // 7. FindAllTestSteps
    public IList<TestStep> FindAllTestSteps() => _repository.TestStepById.Values.ToList();

    // 8. FindAllTestCases
    public IList<TestCase> FindAllTestCases()
    {
        return _repository.TestCaseById.Values.ToList();
    }

    // 9. FindAllTestStepStarted
    public IList<TestStepStarted> FindAllTestStepStarted()
    {
        return _repository.TestStepsStartedByTestCaseStartedId.Values
            .SelectMany(list => list)
            .ToList();
    }

    // 10. FindAllTestStepFinished
    public IList<TestStepFinished> FindAllTestStepFinished()
    {
        return _repository.TestStepsFinishedByTestCaseStartedId.Values
            .SelectMany(list => list)
            .ToList();
    }

    // 11. FindAllTestRunHookStarted
    public IList<TestRunHookStarted> FindAllTestRunHookStarted()
    {
        return _repository.TestRunHookStartedById.Values.ToList();
    }

    // 12. FindAllTestRunHookFinished
    public IList<TestRunHookFinished> FindAllTestRunHookFinished()
    {
        return _repository.TestRunHookFinishedByTestRunHookStartedId.Values.ToList();
    }

    // 13. FindAttachmentsBy(TestStepFinished)
    public IList<Attachment> FindAttachmentsBy(TestStepFinished testStepFinished) =>
        _repository.AttachmentsByTestCaseStartedId.TryGetValue(testStepFinished.TestCaseStartedId, out var attachments)
            ? attachments.Where(a => a.TestStepId == testStepFinished.TestStepId).ToList()
            : new List<Attachment>();

    // 14. FindAttachmentsBy(TestRunHookFinished)
    public IList<Attachment> FindAttachmentsBy(TestRunHookFinished testRunHookFinished)
    {
        if (_repository.AttachmentsByTestRunHookStartedId.TryGetValue(testRunHookFinished.TestRunHookStartedId, out var attachments))
            return new List<Attachment>(attachments);
        return new List<Attachment>();
    }

    // 15. FindHookBy
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

    // 16. FindMeta
    public Meta? FindMeta() => _repository.Meta;

    // 17. FindMostSevereTestStepResultBy(TestCaseStarted)
    public TestStepResult? FindMostSevereTestStepResultBy(TestCaseStarted testCaseStarted)
    {
        var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
        if (finishedSteps.Count == 0)
            return null;
        // Find the TestStepFinished with the most severe status (highest enum value)
        var mostSevere = finishedSteps
            .OrderBy(f => f.TestStepResult.Status)
            .LastOrDefault();
        return mostSevere?.TestStepResult;
    }

    // 18. FindMostSevereTestStepResultBy(TestCaseFinished)
    public TestStepResult? FindMostSevereTestStepResultBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindMostSevereTestStepResultBy(testCaseStarted) : null;
    }

    // 19. FindLocationOf
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

    // 20. FindPickleBy(TestCaseStarted)
    public Pickle? FindPickleBy(TestCaseStarted testCaseStarted)
    {
        var testCase = FindTestCaseBy(testCaseStarted);
        if (testCase != null && _repository.PickleById.TryGetValue(testCase.PickleId, out var pickle))
        {
            return pickle;
        }
        return null;
    }

    // 21. FindPickleBy(TestCaseFinished)
    public Pickle? FindPickleBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    // 22. FindPickleBy(TestCase)
    public Pickle? FindPickleBy(TestCase testCase)
    {
        return testCase != null && _repository.PickleById.TryGetValue(testCase.PickleId, out var pickle) ? pickle : null;
    }

    // 23. FindPickleBy(TestStepStarted)
    public Pickle? FindPickleBy(TestStepStarted testStepStarted)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    // 24. FindPickleBy(TestStepFinished)
    public Pickle? FindPickleBy(TestStepFinished testStepFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepFinished);
        return testCaseStarted != null ? FindPickleBy(testCaseStarted) : null;
    }

    // 25. FindPickleStepBy
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

    // 26. FindSuggestionsBy(PickleStep)
    public IList<Suggestion> FindSuggestionsBy(PickleStep pickleStep)
    {
        if (_repository.SuggestionsByPickleStepId.TryGetValue(pickleStep.Id, out var suggestions))
            return new List<Suggestion>(suggestions);
        return new List<Suggestion>();
    }

    // 27. FindSuggestionsBy(Pickle)
    public IList<Suggestion> FindSuggestionsBy(Pickle pickle)
    {
        var result = new List<Suggestion>();
        foreach (var step in pickle.Steps)
            result.AddRange(FindSuggestionsBy(step));
        return result;
    }

    // 28. FindStepBy(PickleStep)
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

    // 29. FindStepDefinitionsBy
    public IList<StepDefinition> FindStepDefinitionsBy(TestStep testStep)
    {
        if (testStep.StepDefinitionIds != null)
            return testStep.StepDefinitionIds
                .Select(id => _repository.StepDefinitionById.TryGetValue(id, out var def) ? def : null)
                .Where(def => def != null)
                .Cast<StepDefinition>()
                .ToList();
        return new List<StepDefinition>();
    }

    // 30. FindUnambiguousStepDefinitionBy
    public StepDefinition? FindUnambiguousStepDefinitionBy(TestStep testStep)
    {
        if (testStep.StepDefinitionIds != null && testStep.StepDefinitionIds.Count == 1)
            return _repository.StepDefinitionById.TryGetValue(testStep.StepDefinitionIds[0], out var def) ? def : null;
        return null;
    }

    // 31. FindTestCaseBy(TestCaseStarted)
    public TestCase? FindTestCaseBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestCaseById.TryGetValue(testCaseStarted.TestCaseId, out var testCase))
        {
            return testCase;
        }
        return null;
    }

    // 32. FindTestCaseBy(TestCaseFinished)
    public TestCase? FindTestCaseBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    // 33. FindTestCaseBy(TestStepStarted)
    public TestCase? FindTestCaseBy(TestStepStarted testStepStarted)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    // 34. FindTestCaseBy(TestStepFinished)
    public TestCase? FindTestCaseBy(TestStepFinished testStepFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testStepFinished);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    // 35. FindTestCaseDurationBy(TestCaseStarted)
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

    // 36. FindTestCaseDurationBy(TestCaseFinished)
    public System.TimeSpan? FindTestCaseDurationBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestCaseDurationBy(testCaseStarted) : null;
    }

    // 37. FindTestCaseStartedBy(TestStepStarted)
    public TestCaseStarted? FindTestCaseStartedBy(TestStepStarted testStepStarted)
    {
        return _repository.TestCaseStartedById.TryGetValue(testStepStarted.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    // 38. FindTestCaseStartedBy(TestCaseFinished)
    public TestCaseStarted? FindTestCaseStartedBy(TestCaseFinished testCaseFinished)
    {
        return _repository.TestCaseStartedById.TryGetValue(testCaseFinished.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    // 39. FindTestCaseStartedBy(TestStepFinished)
    public TestCaseStarted? FindTestCaseStartedBy(TestStepFinished testStepFinished)
    {
        return _repository.TestCaseStartedById.TryGetValue(testStepFinished.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    // 40. FindTestCaseFinishedBy(TestCaseStarted)
    public TestCaseFinished? FindTestCaseFinishedBy(TestCaseStarted testCaseStarted)
    {
        return _repository.TestCaseFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var finished) ? finished : null;
    }

    // 41. FindTestRunHookFinishedBy(TestRunHookStarted)
    public TestRunHookFinished? FindTestRunHookFinishedBy(TestRunHookStarted testRunHookStarted)
    {
        return _repository.TestRunHookFinishedByTestRunHookStartedId.TryGetValue(testRunHookStarted.Id, out var finished) ? finished : null;
    }

    // 42. FindTestRunHookStartedBy(TestRunHookFinished)
    public TestRunHookStarted? FindTestRunHookStartedBy(TestRunHookFinished testRunHookFinished)
    {
        return _repository.TestRunHookStartedById.TryGetValue(testRunHookFinished.TestRunHookStartedId, out var started) ? started : null;
    }

    // 43. FindTestRunDuration
    public System.TimeSpan? FindTestRunDuration()
    {
        if (_repository.TestRunStarted == null || _repository.TestRunFinished == null)
            return null;
        var start = Converters.ToDateTime(_repository.TestRunStarted.Timestamp);
        var finish = Converters.ToDateTime(_repository.TestRunFinished.Timestamp);
        return finish - start;
    }

    // 44. FindTestRunFinished
    public TestRunFinished? FindTestRunFinished() => _repository.TestRunFinished;

    // 45. FindTestRunStarted
    public TestRunStarted? FindTestRunStarted() => _repository.TestRunStarted;

    // 46. FindTestStepBy(TestStepStarted)
    public TestStep? FindTestStepBy(TestStepStarted testStepStarted)
    {
        return _repository.TestStepById.TryGetValue(testStepStarted.TestStepId, out var testStep) ? testStep : null;
    }

    // 47. FindTestStepBy(TestStepFinished)
    public TestStep? FindTestStepBy(TestStepFinished testStepFinished)
    {
        return _repository.TestStepById.TryGetValue(testStepFinished.TestStepId, out var testStep) ? testStep : null;
    }

    // 48. FindTestStepsStartedBy(TestCaseStarted)
    public IList<TestStepStarted> FindTestStepsStartedBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestStepsStartedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            return new List<TestStepStarted>(steps);
        }
        return new List<TestStepStarted>();
    }

    // 49. FindTestStepsStartedBy(TestCaseFinished)
    public IList<TestStepStarted> FindTestStepsStartedBy(TestCaseFinished testCaseFinished)
    {
        if (_repository.TestStepsStartedByTestCaseStartedId.TryGetValue(testCaseFinished.TestCaseStartedId, out var steps))
        {
            return new List<TestStepStarted>(steps);
        }
        return new List<TestStepStarted>();
    }

    // 50. FindTestStepsFinishedBy(TestCaseStarted)
    public IList<TestStepFinished> FindTestStepsFinishedBy(TestCaseStarted testCaseStarted)
    {
        if (_repository.TestStepsFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            return new List<TestStepFinished>(steps);
        }
        return new List<TestStepFinished>();
    }

    // 51. FindTestStepsFinishedBy(TestCaseFinished)
    public IList<TestStepFinished> FindTestStepsFinishedBy(TestCaseFinished testCaseFinished)
    {
        var testCaseStarted = FindTestCaseStartedBy(testCaseFinished);
        return testCaseStarted != null ? FindTestStepsFinishedBy(testCaseStarted) : new List<TestStepFinished>();
    }

    // 52. FindTestStepFinishedAndTestStepBy(TestCaseStarted)
    public IList<(TestStepFinished, TestStep)> FindTestStepFinishedAndTestStepBy(TestCaseStarted testCaseStarted)
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
}
