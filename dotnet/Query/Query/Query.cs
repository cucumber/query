#nullable enable
using System;
using System.ComponentModel.DataAnnotations;
using System.Collections.Generic;
using System.Linq;
using Io.Cucumber.Messages.Types;
using System.Collections.Concurrent;

namespace Io.Cucumber.Query;

// Ported from io.cucumber.query.Query (Java)
public class Query
{
    // Internal state for queries
    private readonly List<GherkinDocument> _gherkinDocuments = new();
    private readonly List<Pickle> _pickles = new();
    private readonly List<TestCase> _testCases = new();
    private readonly List<TestStepFinished> _testStepFinished = new();
    private readonly List<TestCaseFinished> _testCaseFinished = new();
    private readonly List<TestCaseStarted> _testCaseStarted = new();

    // Additional internal state for queries (to be expanded as needed)
    private readonly ConcurrentDictionary<string, TestCaseStarted> _testCaseStartedById = new();
    private readonly ConcurrentDictionary<string, TestCaseFinished> _testCaseFinishedByTestCaseStartedId = new();
    private readonly ConcurrentDictionary<string, List<TestStepFinished>> _testStepsFinishedByTestCaseStartedId = new();
    private readonly ConcurrentDictionary<string, List<TestStepStarted>> _testStepsStartedByTestCaseStartedId = new();
    private readonly ConcurrentDictionary<string, Pickle> _pickleById = new();
    private readonly ConcurrentDictionary<string, TestCase> _testCaseById = new();
    private readonly ConcurrentDictionary<string, Step> _stepById = new();
    private readonly ConcurrentDictionary<string, TestStep> _testStepById = new();
    private readonly ConcurrentDictionary<string, PickleStep> _pickleStepById = new();
    private readonly ConcurrentDictionary<string, Hook> _hookById = new();
    private readonly ConcurrentDictionary<string, List<Attachment>> _attachmentsByTestCaseStartedId = new();
    private readonly ConcurrentDictionary<object, Lineage> _lineageById = new();
    private Meta? _meta;
    private TestRunStarted? _testRunStarted;
    private TestRunFinished? _testRunFinished;

    public Query() { }

    // Property getters for counts
    public int MostSevereTestStepResultStatusCount => CountMostSevereTestStepResultStatus().Count;
    public int TestCasesStartedCount => FindAllTestCaseStarted().Count;

    // Ported methods (incrementally implemented)
    public IDictionary<TestStepResultStatus, long> CountMostSevereTestStepResultStatus()
    {
        var statusCounts = new Dictionary<TestStepResultStatus, long>();
        foreach (var testCaseStarted in FindAllTestCaseStarted())
        {
            var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
            if (finishedSteps.Count == 0)
                continue;
            // Find the most severe status (lowest enum value)
            var mostSevere = finishedSteps
                .Select(f => f.TestStepResult.Status)
                .Min();
            if (statusCounts.ContainsKey(mostSevere))
                statusCounts[mostSevere]++;
            else
                statusCounts[mostSevere] = 1;
        }
        return statusCounts;
    }

    public IList<Pickle> FindAllPickles() => _pickleById.Values.OrderBy(p => p.Id).ToList();

    public IList<PickleStep> FindAllPickleSteps() => _pickleStepById.Values.OrderBy(ps => ps.Id).ToList();

    public IList<TestCaseStarted> FindAllTestCaseStarted() => _testCaseStartedById.Values
        .OrderBy(tcs => tcs.Timestamp, new TimestampComparer())
        .ThenBy(tcs => tcs.Id)
        .Where(tcs => !FindTestCaseFinishedBy(tcs)?.WillBeRetried ?? true) // Exclude finished cases that will be retried
        .ToList();

    public IList<TestStep> FindAllTestSteps() => _testStepById.Values.OrderBy(ts => ts.Id).ToList();

    public IDictionary<Feature?, List<TestCaseStarted>> FindAllTestCaseStartedGroupedByFeature()
    {
        // Group TestCaseStarted by Feature (null if not found)
        return FindAllTestCaseStarted()
            .GroupBy(tcs => FindFeatureBy(tcs))
            .ToDictionary(g => g.Key, g => g.ToList());
    }

    public Meta? FindMeta() => _meta;
    public TestRunStarted? FindTestRunStarted() => _testRunStarted;
    public TestRunFinished? FindTestRunFinished() => _testRunFinished;

    public IList<Attachment> FindAttachmentsBy(TestStepFinished testStepFinished) =>
        _attachmentsByTestCaseStartedId.TryGetValue(testStepFinished.TestCaseStartedId, out var attachments)
            ? attachments.Where(a => a.TestStepId == testStepFinished.TestStepId).ToList()
            : new List<Attachment>();

    public Feature? FindFeatureBy(TestCaseStarted testCaseStarted)
    {
        // Find the TestCase for this TestCaseStarted
        if (_testCaseById.TryGetValue(testCaseStarted.TestCaseId, out var testCase))
        {
            // Find the Pickle for this TestCase
            if (_pickleById.TryGetValue(testCase.PickleId, out var pickle))
            {
                // Find the GherkinDocument for this Pickle
                var doc = _gherkinDocuments.FirstOrDefault(d => d.Uri == pickle.Uri);
                return doc?.Feature;
            }
        }
        return null;
    }

    public Hook? FindHookBy(TestStep testStep)
    {
        // Java: testStep.getHookId().map(hookById::get)
        if (!string.IsNullOrEmpty(testStep.HookId) && _hookById.TryGetValue(testStep.HookId, out var hook))
        {
            return hook;
        }
        return null;
    }

    public Pickle? FindPickleBy(TestCaseStarted testCaseStarted)
    {
        // Java: findTestCaseBy(testCaseStarted).map(TestCase::getPickleId).map(pickleById::get)
        var testCase = FindTestCaseBy(testCaseStarted);
        if (testCase != null && _pickleById.TryGetValue(testCase.PickleId, out var pickle))
        {
            return pickle;
        }
        return null;
    }

    public Pickle? FindPickleBy(TestStepStarted testStepStarted)
    {
        // Java: findTestCaseBy(testStepStarted).map(TestCase::getPickleId).map(pickleById::get)
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        if (testCaseStarted != null)
        {
            return FindPickleBy(testCaseStarted);
        }
        return null;
    }

    public TestCase? FindTestCaseBy(TestCaseStarted testCaseStarted)
    {
        // Java: testCaseById.get(testCaseStarted.getTestCaseId())
        if (_testCaseById.TryGetValue(testCaseStarted.TestCaseId, out var testCase))
        {
            return testCase;
        }
        return null;
    }

    public TestCaseStarted? FindTestCaseStartedBy(TestStepStarted testStepStarted)
    {
        // Java: testCaseStartedById.get(testStepStarted.getTestCaseStartedId())
        return _testCaseStartedById.TryGetValue(testStepStarted.TestCaseStartedId, out var tcs) ? tcs : null;
    }

    public TestCase? FindTestCaseBy(TestStepStarted testStepStarted)
    {
        // Java: findTestCaseStartedBy(testStepStarted).flatMap(this::findTestCaseBy)
        var testCaseStarted = FindTestCaseStartedBy(testStepStarted);
        return testCaseStarted != null ? FindTestCaseBy(testCaseStarted) : null;
    }

    public TestStep? FindTestStepBy(TestStepStarted testStepStarted)
    {
        // Java: testStepById.get(testStepStarted.getTestStepId())
        return _testStepById.TryGetValue(testStepStarted.TestStepId, out var testStep) ? testStep : null;
    }

    public TestStep? FindTestStepBy(TestStepFinished testStepFinished)
    {
        // Java: testStepById.get(testStepFinished.getTestStepId())
        return _testStepById.TryGetValue(testStepFinished.TestStepId, out var testStep) ? testStep : null;
    }

    public PickleStep? FindPickleStepBy(TestStep testStep)
    {
        // Java: testStep.getPickleStepId().map(pickleStepById::get)
        if (!string.IsNullOrEmpty(testStep.PickleStepId))
        {
            if (_pickleStepById.TryGetValue(testStep.PickleStepId, out var pickleStep))
            {
                return pickleStep;
            }
        }
        return null;
    }

    public Step? FindStepBy(PickleStep pickleStep)
    {
        // Java: String stepId = pickleStep.getAstNodeIds().get(0); stepById.get(stepId)
        if (pickleStep.AstNodeIds != null && pickleStep.AstNodeIds.Count > 0)
        {
            var stepId = pickleStep.AstNodeIds[0];
            if (!string.IsNullOrEmpty(stepId) && _stepById.TryGetValue(stepId, out var step))
            {
                return step;
            }
        }
        return null;
    }

    public System.TimeSpan? FindTestCaseDurationBy(TestCaseStarted testCaseStarted)
    {
        var started = testCaseStarted.Timestamp;
        var finished = FindTestCaseFinishedBy(testCaseStarted)?.Timestamp;
        if (finished != null)
        {
            var startTime = TimestampToDateTimeOffset(started);
            var finishTime = TimestampToDateTimeOffset(finished);
            return finishTime - startTime;
        }
        return null;
    }

    public TestCaseFinished? FindTestCaseFinishedBy(TestCaseStarted testCaseStarted)
    {
        // Java: testCaseFinishedByTestCaseStartedId.get(testCaseStarted.getId())
        return _testCaseFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var finished) ? finished : null;
    }

    public System.TimeSpan? FindTestRunDuration()
    {
        // Java: if (testRunStarted == null || testRunFinished == null) return Optional.empty();
        if (_testRunStarted == null || _testRunFinished == null)
            return null;
        var start = TimestampToDateTimeOffset(_testRunStarted.Timestamp);
        var finish = TimestampToDateTimeOffset(_testRunFinished.Timestamp);
        return finish - start;
    }

    public IList<TestStepStarted> FindTestStepsStartedBy(TestCaseStarted testCaseStarted)
    {
        // Java: testStepsStartedByTestCaseStartedId.getOrDefault(testCaseStarted.getId(), emptyList())
        if (_testStepsStartedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            // Return a copy for concurrency safety
            return new List<TestStepStarted>(steps);
        }
        return new List<TestStepStarted>();
    }

    public IList<TestStepFinished> FindTestStepsFinishedBy(TestCaseStarted testCaseStarted)
    {
        // Java: testStepsFinishedByTestCaseStartedId.getOrDefault(testCaseStarted.getId(), emptyList())
        if (_testStepsFinishedByTestCaseStartedId.TryGetValue(testCaseStarted.Id, out var steps))
        {
            // Return a copy for concurrency safety
            return new List<TestStepFinished>(steps);
        }
        return new List<TestStepFinished>();
    }

    public IList<(TestStepFinished, TestStep)> FindTestStepFinishedAndTestStepBy(TestCaseStarted testCaseStarted)
    {
        // Java: findTestStepsFinishedBy(testCaseStarted).stream()
        //   .map(testStepFinished -> findTestStepBy(testStepFinished).map(testStep -> new SimpleEntry<>(testStepFinished, testStep)))
        //   .filter(Optional::isPresent)
        //   .map(Optional::get)
        //   .collect(toList());
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

    public TestStepResult? FindMostSevereTestStepResultBy(TestCaseStarted testCaseStarted)
    {
        var finishedSteps = FindTestStepsFinishedBy(testCaseStarted);
        if (finishedSteps.Count == 0)
            return null;
        // Find the TestStepFinished with the most severe status (lowest enum value)
        var mostSevere = finishedSteps
            .OrderBy(f => f.TestStepResult.Status)
            .FirstOrDefault();
        return mostSevere?.TestStepResult;
    }

    private static DateTimeOffset TimestampToDateTimeOffset(Timestamp timestamp)
    {
        // Java: Convertor.toInstant(timestamp)
        // C#: Timestamp has Seconds and Nanos
        var dateTime = DateTimeOffset.FromUnixTimeSeconds(timestamp.Seconds);
        return dateTime.AddTicks(timestamp.Nanos / 100);
    }

    // Update methods for each message type (ported from Java)
    internal void UpdateAttachment(Attachment attachment)
    {
        if (attachment.TestCaseStartedId != null)
        {
            _attachmentsByTestCaseStartedId.AddOrUpdate(
                attachment.TestCaseStartedId,
                _ => new List<Attachment> { attachment },
                (_, list) => { list.Add(attachment); return list; });
        }
    }

    internal void UpdateHook(Hook hook)
    {
        _hookById[hook.Id] = hook;
    }

    internal void UpdateTestCaseStarted(TestCaseStarted testCaseStarted)
    {
        _testCaseStartedById[testCaseStarted.Id] = testCaseStarted;
        _testCaseStarted.Add(testCaseStarted);
    }

    internal void UpdateTestCase(TestCase testCase)
    {
        _testCaseById[testCase.Id] = testCase;
        _testCases.Add(testCase);
        foreach (var testStep in testCase.TestSteps)
        {
            _testStepById[testStep.Id] = testStep;
        }
    }

    internal void UpdatePickle(Pickle pickle)
    {
        _pickleById[pickle.Id] = pickle;
        _pickles.Add(pickle);
        foreach (var step in pickle.Steps)
        {
            _pickleStepById[step.Id] = step;
        }
    }

    internal void UpdateGherkinDocument(GherkinDocument document)
    {
        _gherkinDocuments.Add(document);
        if (document.Feature != null)
        {
            UpdateFeature(document.Feature);
        }
        // Lineage population would go here if implemented
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
        _testStepsStartedByTestCaseStartedId.AddOrUpdate(
            testStepStarted.TestCaseStartedId,
            _ => new List<TestStepStarted> { testStepStarted },
            (_, list) => { list.Add(testStepStarted); return list; });
    }

    internal void UpdateTestStepFinished(TestStepFinished testStepFinished)
    {
        _testStepsFinishedByTestCaseStartedId.AddOrUpdate(
            testStepFinished.TestCaseStartedId,
            _ => new List<TestStepFinished> { testStepFinished },
            (_, list) => { list.Add(testStepFinished); return list; });
        _testStepFinished.Add(testStepFinished);
    }

    internal void UpdateTestCaseFinished(TestCaseFinished testCaseFinished)
    {
        _testCaseFinishedByTestCaseStartedId[testCaseFinished.TestCaseStartedId] = testCaseFinished;
        _testCaseFinished.Add(testCaseFinished);
    }

    internal void UpdateTestRunFinished(TestRunFinished testRunFinished)
    {
        _testRunFinished = testRunFinished;
    }

    internal void UpdateTestRunStarted(TestRunStarted testRunStarted)
    {
        _testRunStarted = testRunStarted;
    }

    internal void UpdateScenario(Scenario scenario)
    {
        UpdateSteps(scenario.Steps);
    }

    internal void UpdateSteps(IList<Step> steps)
    {
        foreach (var step in steps)
        {
            _stepById[step.Id] = step;
        }
    }

    internal void UpdateMeta(Meta meta)
    {
        _meta = meta;
    }

    public void Update(Envelope envelope)
    {
        if (envelope.Meta != null) UpdateMeta(envelope.Meta);
        if (envelope.TestRunStarted != null) UpdateTestRunStarted(envelope.TestRunStarted);
        if (envelope.TestRunFinished != null) UpdateTestRunFinished(envelope.TestRunFinished);
        if (envelope.TestCaseStarted != null) UpdateTestCaseStarted(envelope.TestCaseStarted);
        if (envelope.TestCaseFinished != null) UpdateTestCaseFinished(envelope.TestCaseFinished);
        if (envelope.TestStepStarted != null) UpdateTestStepStarted(envelope.TestStepStarted);
        if (envelope.TestStepFinished != null) UpdateTestStepFinished(envelope.TestStepFinished);
        if (envelope.GherkinDocument != null) UpdateGherkinDocument(envelope.GherkinDocument);
        if (envelope.Pickle != null) UpdatePickle(envelope.Pickle);
        if (envelope.TestCase != null) UpdateTestCase(envelope.TestCase);
        if (envelope.Hook != null) UpdateHook(envelope.Hook);
        if (envelope.Attachment != null) UpdateAttachment(envelope.Attachment);
    }

    // FindLineageBy methods
    public Lineage? FindLineageBy(GherkinDocument element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Feature element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Rule element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Scenario element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Examples element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(TableRow element)
    {
        _lineageById.TryGetValue(element, out var lineage);
        return lineage;
    }

    public Lineage? FindLineageBy(Pickle pickle)
    {
        _lineageById.TryGetValue(pickle, out var lineage);
        return lineage;
    }

    public string FindNameOf(GherkinDocument element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(Feature element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(Rule element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(Scenario element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(Examples element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(TableRow element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        return GetNameFromStrategy(element, lineage, namingStrategy);
    }

    public string FindNameOf(Pickle element, NamingStrategy namingStrategy)
    {
        if (element == null) return string.Empty;
        var lineage = FindLineageBy(element);
        var result = namingStrategy.Reduce(lineage, element);
        if (result == null)
        {
            throw new ArgumentException("Element was not part of this query object");
        }
        return result;

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

    // Placeholder for actual naming strategy logic
    private string GetNameFromStrategy(object element, Lineage? lineage, NamingStrategy namingStrategy)
    {
        var result = namingStrategy.Reduce(lineage);
        if (result == null)
        {
            throw new ArgumentException("Element was not part of this query object");
        }
        return result;
    }
}
