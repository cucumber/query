# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

## [13.6.0] - 2025-08-11
### Changed
- [Java] Replace redundant concurrent hashmap with regular hashmap ([#89](https://github.com/cucumber/query/pull/89))

## [13.5.0] - 2025-07-10
### Added
- Add `findStepDefinitionsBy` and `findUnambiguousStepDefinitionBy` ([#80](https://github.com/cucumber/query/pull/80))

## [13.4.0] - 2025-07-07
### Added
- Update dependency messages to v28

## [13.3.0] - 2025-06-10
### Added
- Made `Lineage` APIs public ([#76](https://github.com/cucumber/query/pull/76))
- New method `findPickleBy(TestStepStarted)`  ([#76](https://github.com/cucumber/query/pull/76))
- New method `findTestCaseBy(TestStepStarted)` ([#76](https://github.com/cucumber/query/pull/76))
- New method `findTestCaseStartedBy(TestStepStarted)` ([#76](https://github.com/cucumber/query/pull/76))
- New method `findTestStepBy(TestStepStarted)` ([#76](https://github.com/cucumber/query/pull/76))
- New method `findTestStepsStartedBy(TestCaseStarted)` ([#76](https://github.com/cucumber/query/pull/76))

### Fixed
- `Query.findAllTestCaseStarted` orders events by `timestamp` and `id` ([#76](https://github.com/cucumber/query/pull/76))

## [13.2.0] - 2025-02-02
### Changed
- Update dependency messages to support v27

## [13.1.0] - 2025-01-14
### Added
- New method `findAttachmentsBy(TestStepFinished)` ([#67](https://github.com/cucumber/query/pull/67))
- New method `findHookBy(TestStep)` ([#67](https://github.com/cucumber/query/pull/67))
- New method `findMeta()` ([#67](https://github.com/cucumber/query/pull/67))

### Fixed
- [JavaScript] Attachments are not presumed to have a related test step ([#67](https://github.com/cucumber/query/pull/67))

## [13.0.3] - 2024-12-22
### Fixed
- [JavaScript] Remove dependency on `assert` package ([#66](https://github.com/cucumber/query/pull/66))

## [13.0.2] - 2024-11-15
### Fixed
- [Java] Don't expose `EnumMap` implementation detail

## [13.0.1] - 2024-11-14
### Fixed
- [Java] `countTestCasesStarted` now accounts for retried test cases ([#65](https://github.com/cucumber/query/pull/65))
- [JavaScript] `Lineage` and related symbols now exported on entry point ([#65](https://github.com/cucumber/query/pull/65))

## [13.0.0] - 2024-11-14
### Added
- New methods in JavaScript implementation to match Java ([#62](https://github.com/cucumber/query/pull/62))
- Update dependency @cucumber/messages to v26  ((#52)[https://github.com/cucumber/query/pull/52])
- Update dependency io.cucumber:messages up to v26 ((#53)[https://github.com/cucumber/query/pull/53])

### Changed
- BREAKING CHANGE: `countMostSevereTestStepResultStatus` now returns `EnumMap` with all statuses regardless of count ([#62](https://github.com/cucumber/query/pull/62))
- BREAKING CHANGE: `findAllTestCaseStarted` now omits `TestCaseStarted` messages where there is or will be another attempt ([#62](https://github.com/cucumber/query/pull/62))
- BREAKING CHANGE: Rename `findMostSevereTestStepResulBy` to `findMostSevereTestStepResultBy` ([#62](https://github.com/cucumber/query/pull/62))

### Removed
- BREAKING CHANGE: Remove support for Node.js 16.x and 17.x ([#62](https://github.com/cucumber/query/pull/62))

## [12.2.0] - 2024-06-22
### Changed
- Include pickle name if parameterized ((#44)[https://github.com/cucumber/query/pull/44])

### Fixed
- java: Require all arguments to the naming strategy builder to be non-null

## [12.1.2] - 2024-04-05
### Fixed
- java: Do not expose `SimpleEntry` as part of the public API

## [12.1.1] - 2024-04-05
### Fixed
- java: Fix Javadoc for release

## [12.1.0] - 2024-04-05
### Added
- java: implementation of query ([#39](https://github.com/cucumber/query/pull/39))

## [12.0.2] - 2024-03-26
### Fixed
- Correct repo URL in `package.json`

## [12.0.1] - 2022-11-21
### Added

### Changed

### Deprecated

### Fixed

### Removed

## [12.0.0] - 2022-06-01
### Changed
- Bump `@cucumber/messages` to v19.0.0

## [11.0.0] - 2021-07-08
### Changed
- Bump `@cucumber/messages` to v17.0.0

### Fixed
- `Query#getPickleStepAttachments`
- `Query#getPickleStepTestStepResults`
- `Query#getPickleTestStepResults`
- `Query#getStatusCounts`
- `Query#getTestStepResults`
- `Query#getTestStepsAttachments`
- Methods that return/map step results now include results from _only the last attempt_ where there have been retries ([#1631](https://github.com/cucumber/common/pull/1631)). Affects methods:

## [10.1.0] - 2021-05-31
### Added
- New `Query#getStatusCounts(pickleIds: string[])` method which calculates a summary
of a run.

## [10.0.0] - 2021-05-17
### Changed
- Upgrade to gherkin 19.0.0
- Upgrade to messages 16.0.0

### Removed
- [JavaScript] Removed `Query#getWorstTestStepResult` method. Use `getWorstTestStepResult`
from `@cucumber/messages` instead.

## [9.0.2] - 2021-04-06
### Fixed
- [JavaScript] Fix issue with compiled files not appearing in published package
([#1452](https://github.com/cucumber/cucumber/pull/1452))

## [9.0.1] - 2021-04-03
### Fixed
- Fixed a snafu with the 9.0.0 release

## [9.0.0] - 2021-03-29
### Changed
- Upgrade to messages 15.0.0

## [8.0.0] - 2021-02-07
### Changed
- Upgrade to messages 14.0.0

## [7.0.1] - 2020-12-17
### Fixed
- Removed unneeded `@cucumber/gherkin` dependency

## [7.0.0] - 2020-08-07
### Changed
- Update `messages` to 13.0.1

## [6.1.0] - 2020-06-29
### Added
- Add `getBeforeHookSteps` and `getAfterHookSteps`
- Add `getTestStepResults`
- Add `getHook`

## [6.0.0] - 2020-04-14
### Changed
- Upgrade to messages 12.0.0
- Upgrade to gherkin 13.0.0

## [5.0.0] - 2020-03-31
### Added
- Add `QueryStream`
- Major bump of gherkin and messages

## [4.0.0] - 2020-03-02
### Changed
- Upgraded gherkin

### Fixed
- Report `Status.UNKNOWN` when status is not known
- Add `gherkin` as a runtime dependency

## [3.0.0] - 2020-02-14
### Changed
- Rolled `TestResultsQuery` and `StepMatchArgumentsQuery` into a new, single `Query` class.
- Upgraded fake-cucumber, gherkin and messages

## [2.0.0] - 2020-01-22
### Changed
- [JavaScript] the API been rewritten

## [1.1.1] - 2020-01-10
### Changed
- [JavaScript] changed module name to `@cucumber/query`

## [1.1.0] - 2019-12-10
### Changed
- Something changed, but we didn't record what. Look at the diff!

## [1.0.0] - 2019-11-15
### Added
- First JavaScript implementation

[Unreleased]: https://github.com/cucumber/cucumber/compare/query/v13.6.0...HEAD
[13.6.0]: https://github.com/cucumber/cucumber/compare/query/v13.5.0...v13.6.0
[13.5.0]: https://github.com/cucumber/cucumber/compare/query/v13.4.0...v13.5.0
[13.4.0]: https://github.com/cucumber/cucumber/compare/query/v13.3.0...v13.4.0
[13.3.0]: https://github.com/cucumber/cucumber/compare/query/v13.2.0...v13.3.0
[13.2.0]: https://github.com/cucumber/cucumber/compare/query/v13.1.0...v13.2.0
[13.1.0]: https://github.com/cucumber/cucumber/compare/query/v13.0.3...v13.1.0
[13.0.3]: https://github.com/cucumber/cucumber/compare/query/v13.0.2...v13.0.3
[13.0.2]: https://github.com/cucumber/cucumber/compare/query/v13.0.1...v13.0.2
[13.0.1]: https://github.com/cucumber/cucumber/compare/query/v13.0.0...v13.0.1
[13.0.0]: https://github.com/cucumber/cucumber/compare/query/v12.2.0...v13.0.0
[12.2.0]: https://github.com/cucumber/cucumber/compare/query/v12.1.2...v12.2.0
[12.1.2]: https://github.com/cucumber/cucumber/compare/query/v12.1.1...v12.1.2
[12.1.1]: https://github.com/cucumber/cucumber/compare/query/v12.1.0...v12.1.1
[12.1.0]: https://github.com/cucumber/cucumber/compare/query/v12.0.2...v12.1.0
[12.0.2]: https://github.com/cucumber/cucumber/compare/query/v12.0.1...v12.0.2
[12.0.1]: https://github.com/cucumber/cucumber/compare/query/v12.0.0...v12.0.1
[12.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v11.0.0...query/v12.0.0
[11.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v10.1.0...query/v11.0.0
[10.1.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v10.0.0...query/v10.1.0
[10.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v9.0.2...query/v10.0.0
[9.0.2]: https://github.com/cucumber/cucumber/compare/cucumber-query/v9.0.1...query/v9.0.2
[9.0.1]: https://github.com/cucumber/cucumber/compare/cucumber-query/v9.0.0...query/v9.0.1
[9.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v8.0.0...query/v9.0.0
[8.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v7.0.1...query/v8.0.0
[7.0.1]: https://github.com/cucumber/cucumber/compare/cucumber-query/v7.0.0...query/v7.0.1
[7.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v6.1.0...query/v7.0.0
[6.1.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v6.0.0...query/v6.1.0
[6.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v5.0.0...query/v6.0.0
[5.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v4.0.0...query/v5.0.0
[4.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v3.0.0...query/v4.0.0
[3.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v2.0.0...query/v3.0.0
[2.0.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v1.1.1...query/v2.0.0
[1.1.1]: https://github.com/cucumber/cucumber/compare/cucumber-query/v1.1.0...query/v1.1.1
[1.1.0]: https://github.com/cucumber/cucumber/compare/cucumber-query/v1.0.0...cucumber-query/v1.1.0
[1.0.0]: https://github.com/cucumber/cucumber/releases/tag/cucumber-query/v1.0.0
