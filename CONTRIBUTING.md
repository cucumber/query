# Contributing

For general guidance on contributing to Cucumber, see https://github.com/cucumber/.github/blob/main/CONTRIBUTING.md

## Adding or changing query methods

This is a polyglot repo with several languages adhering to a common suite of
acceptance tests. A change should be made consistently across all languages.
Currently, the list is:

- Java (reference)
- JavaScript
- C# (.NET)
- Ruby

Java is the reference implementation in the sense that it is responsible for
generating the fixtures that are used in the acceptance tests to verify all
implementations.

So your playbook for adding a method would be something like:

1. Add the method in `main/**/query/Query.java` with test(s) in `test/**/query/QueryTest.java`
2. Extend `test/**/query/QueryAcceptanceTest.java` to include verifications for the new
   method
3. Run `QueryAcceptanceTest::updateExpectedQueryResultFiles` to regenerate the
   fixtures
4. Implement other languages

## Types

Choosing which type to use in another language based on what we did in Java is
an inexact science. This table defines all the decisions we've made so far:

| Java          | JavaScript           | C#                 | Ruby          |
|---------------|----------------------|--------------------|---------------|
| `Optional<T>` | `T \| undefined`[^1] | `T?`               | `true\|false` |
| `List<T>`     | `ReadonlyArray<T>`   | `List<T>`          | `N/A` [^2]    |
| `Map<K, V>`   | `Map<K, V>`          | `Dictionary<K, V>` | `Hash<K, V>`  |
| `Map<K, V>`   | `Record<K, V>`       | `Dictionary<K, V>` | `Hash<K, V>`  |

[^1]: See <https://github.com/sindresorhus/meta/discussions/7>

[^2]: This only applies to `Lineage` which is something that was never ported and not required
in Ruby. `Lineage` as a concept was only needed whilst `Pickle` did not contain the location
attribute (Which was later added)
