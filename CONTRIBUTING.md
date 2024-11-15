# Contributing

For general guidance on contributing to Cucumber, see https://github.com/cucumber/.github/blob/main/CONTRIBUTING.md

## Adding or changing query methods

This is a polyglot repo with several languages adhering to a common suite of acceptance tests. A change should be made consistently across all languages. Currently, the list is:

- Java (reference)
- JavaScript

Java is the reference implementation in the sense that it is responsible for generating the fixtures that are used in the acceptance tests to verify all implementations.

So your playbook for adding a method would be something like:

1. Add the method in `Query.Java` with test(s) in `QueryTest.java`
2. Extend `QueryAcceptanceTest.java` to include verifications for the new method
3. Run `QueryAcceptanceTest::updateExpectedQueryResultFiles` to regenerate the fixtures
4. Implement other languages

## Types

Choosing which type to use in another language based on what we did in Java is an inexact science. This table defines all the decisions we've made so far:

| Java                | JavaScript              |
|---------------------|-------------------------|
| `Optional<T>`       | `T \| undefined`[^1]    |
| `List<T>`           | `ReadonlyArray<T>`      | 
| `Map<K, V>`         | `Map<K, V>`             | 
| `Map<K, V>`         | `Record<K, V>`          | 
| `List<Entry<T, V>>` | `ReadonlyArray<[T, V]>` |

[^1]: See <https://github.com/sindresorhus/meta/discussions/7>