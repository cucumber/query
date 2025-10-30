#nullable enable

using Io.Cucumber.Messages.Types;
using System;
using System.ComponentModel.DataAnnotations;

namespace Io.Cucumber.Query;

/// <summary>
/// A structure containing all ancestors of a given
/// <see cref="GherkinDocument"/> element or <see cref="Pickle"/>.
/// </summary>
/// <remarks>
/// See <see cref="LineageReducer"/>.
/// </remarks>
public class Lineage : IEquatable<Lineage>
{
    private readonly GherkinDocument _document;
    private readonly Feature? _feature;
    private readonly Rule? _rule;
    private readonly Scenario? _scenario;
    private readonly Examples? _examples;
    private readonly int? _examplesIndex;
    private readonly TableRow? _example;
    private readonly int? _exampleIndex;

    internal Lineage([Required] GherkinDocument document) : this(document, null, null, null, null, null, null, null)
    {
    }

    internal Lineage(Lineage parent, Feature feature)
        : this(parent._document, feature, null, null, null, null, null, null) { }

    internal Lineage(Lineage parent, Rule rule)
        : this(parent._document, parent._feature, rule, null, null, null, null, null) { }

    internal Lineage(Lineage parent, Scenario scenario)
        : this(parent._document, parent._feature, parent._rule, scenario, null, null, null, null) { }

    internal Lineage(Lineage parent, Examples examples, int examplesIndex)
        : this(parent._document, parent._feature, parent._rule, parent._scenario, examples, examplesIndex, null, null) { }

    internal Lineage(Lineage parent, TableRow example, int exampleIndex)
        : this(parent._document, parent._feature, parent._rule, parent._scenario, parent._examples, parent._examplesIndex, example, exampleIndex) { }

    private Lineage(
        [Required] GherkinDocument document,
        Feature? feature,
        Rule? rule,
        Scenario? scenario,
        Examples? examples,
        int? examplesIndex,
        TableRow? example,
        int? exampleIndex)
    {
        _document = document ?? throw new ArgumentNullException(nameof(document));
        _feature = feature;
        _rule = rule;
        _scenario = scenario;
        _examples = examples;
        _examplesIndex = examplesIndex;
        _example = example;
        _exampleIndex = exampleIndex;
    }

    public GherkinDocument Document => _document;
    public Feature? Feature => _feature;
    public Rule? Rule => _rule;
    public Scenario? Scenario => _scenario;
    public Examples? Examples => _examples;
    public TableRow? Example => _example;
    public int? ExamplesIndex => _examplesIndex;
    public int? ExampleIndex => _exampleIndex;

    public bool Equals(Lineage? other)
    {
        if (ReferenceEquals(this, other)) return true;
        if (other is null) return false;
        return Equals(_document, other._document)
            && Equals(_feature, other._feature)
            && Equals(_rule, other._rule)
            && Equals(_scenario, other._scenario)
            && Equals(_examples, other._examples)
            && Equals(_example, other._example)
            && Equals(_examplesIndex, other._examplesIndex)
            && Equals(_exampleIndex, other._exampleIndex);
    }

    public override bool Equals(object? obj) => Equals(obj as Lineage);

    public override int GetHashCode()
    {
        return (_document, _feature, _rule, _scenario, _examples, _example, _examplesIndex, _exampleIndex).GetHashCode();
    }
}