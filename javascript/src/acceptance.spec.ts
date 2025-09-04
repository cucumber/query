import assert from 'node:assert'
import fs from 'node:fs'
import * as path from 'node:path'
import { pipeline, Writable } from 'node:stream'
import util from 'node:util'

// eslint-disable-next-line n/no-extraneous-import
import { NdjsonToMessageStream } from '@cucumber/message-streams'
import { Envelope } from '@cucumber/messages'

import {
  namingStrategy,
  NamingStrategyExampleName,
  NamingStrategyFeatureName,
  NamingStrategyLength,
} from './Lineage'
import Query from './Query'

const asyncPipeline = util.promisify(pipeline)

describe('Acceptance Tests', async () => {
  const sources = [
    path.join(__dirname, '../../testdata/attachments.ndjson'),
    path.join(__dirname, '../../testdata/empty.ndjson'),
    path.join(__dirname, '../../testdata/hooks.ndjson'),
    path.join(__dirname, '../../testdata/minimal.ndjson'),
    path.join(__dirname, '../../testdata/rules.ndjson'),
    path.join(__dirname, '../../testdata/examples-tables.ndjson'),
  ]
  const queries: Queries = {
    countMostSevereTestStepResultStatus: (query: Query) =>
      query.countMostSevereTestStepResultStatus(),
    countTestCasesStarted: (query: Query) => query.countTestCasesStarted(),
    findAllPickles: (query: Query) => query.findAllPickles().length,
    findAllPickleSteps: (query: Query) => query.findAllPickleSteps().length,
    findAllTestCaseStarted: (query: Query) => query.findAllTestCaseStarted().length,
    findAllTestSteps: (query: Query) => query.findAllTestSteps().length,
    findAllTestCaseStartedGroupedByFeature: (query: Query) =>
      [...query.findAllTestCaseStartedGroupedByFeature().entries()].map(
        ([feature, testCaseStarteds]) => [
          feature.name,
          testCaseStarteds.map((testCaseStarted) => testCaseStarted.id),
        ]
      ),
    findAttachmentsBy: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .map((testCaseStarted) => query.findTestStepsFinishedBy(testCaseStarted))
        .map((testStepFinisheds) =>
          testStepFinisheds.map((testStepFinished) => query.findAttachmentsBy(testStepFinished))
        )
        .flat(2)
        .map((attachment) => [
          attachment.testStepId,
          attachment.testCaseStartedId,
          attachment.mediaType,
          attachment.contentEncoding,
        ]),
    findFeatureBy: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .map((testCaseStarted) => query.findFeatureBy(testCaseStarted))
        .map((feature) => feature?.name),
    findHookBy: (query: Query) =>
      query
        .findAllTestSteps()
        .map((testStep) => query.findHookBy(testStep))
        .map((hook) => hook?.id)
        .filter((value) => !!value),
    findMeta: (query: Query) => query.findMeta()?.implementation?.name,
    findMostSevereTestStepResultBy: (query: Query) => {
      return {
        testCaseStarted: query
          .findAllTestCaseStarted()
          .map((testCaseStarted) => query.findMostSevereTestStepResultBy(testCaseStarted))
          .map((testStepResult) => testStepResult?.status)
          .filter((value) => value),
        testCaseFinished: query
          .findAllTestCaseFinished()
          .map((testCaseStarted) => query.findMostSevereTestStepResultBy(testCaseStarted))
          .map((testStepResult) => testStepResult?.status)
          .filter((value) => value),
      }
    },
    findNameOf: (query: Query) => {
      return {
        long: query
          .findAllPickles()
          .map((pickle) => query.findNameOf(pickle, namingStrategy(NamingStrategyLength.LONG))),
        excludeFeatureName: query
          .findAllPickles()
          .map((pickle) =>
            query.findNameOf(
              pickle,
              namingStrategy(NamingStrategyLength.LONG, NamingStrategyFeatureName.EXCLUDE)
            )
          ),
        longPickleName: query
          .findAllPickles()
          .map((pickle) =>
            query.findNameOf(
              pickle,
              namingStrategy(
                NamingStrategyLength.LONG,
                NamingStrategyFeatureName.INCLUDE,
                NamingStrategyExampleName.PICKLE
              )
            )
          ),
        short: query
          .findAllPickles()
          .map((pickle) => query.findNameOf(pickle, namingStrategy(NamingStrategyLength.SHORT))),
        shortPickleName: query
          .findAllPickles()
          .map((pickle) =>
            query.findNameOf(
              pickle,
              namingStrategy(
                NamingStrategyLength.SHORT,
                NamingStrategyFeatureName.INCLUDE,
                NamingStrategyExampleName.PICKLE
              )
            )
          ),
      }
    },
    findLocationOf: (query: Query) =>
      query.findAllPickles().map((pickle) => query.findLocationOf(pickle)),
    findPickleBy: (query: Query) => {
      return {
        testCaseStarted: query
          .findAllTestCaseStarted()
          .map((testCaseStarted) => query.findPickleBy(testCaseStarted))
          .map((pickle) => pickle?.name),
        testCaseFinished: query
          .findAllTestCaseFinished()
          .map((testCaseFinished) => query.findPickleBy(testCaseFinished))
          .map((pickle) => pickle?.name),
      }
    },
    findPickleStepBy: (query: Query) =>
      query
        .findAllTestSteps()
        .map((testStep) => query.findPickleStepBy(testStep))
        .map((pickleStep) => pickleStep?.text)
        .filter((value) => !!value),
    findStepBy: (query: Query) =>
      query
        .findAllPickleSteps()
        .map((pickleStep) => query.findStepBy(pickleStep))
        .map((step) => step?.text),
    findStepDefinitionsBy: (query: Query) =>
      query
        .findAllTestSteps()
        .map((pickleStep) =>
          query.findStepDefinitionsBy(pickleStep).map((stepDefinition) => stepDefinition?.id)
        ),
    findUnambiguousStepDefinitionBy: (query: Query) =>
      query
        .findAllTestSteps()
        .map((pickleStep) => query.findUnambiguousStepDefinitionBy(pickleStep))
        .filter((stepDefinition) => !!stepDefinition)
        .map((stepDefinition) => stepDefinition.id),
    findTestCaseBy: (query: Query) => {
      return {
        testCaseStarted: query
          .findAllTestCaseStarted()
          .map((testCaseStarted) => query.findTestCaseBy(testCaseStarted))
          .map((testCase) => testCase?.id),
        testCaseFinished: query
          .findAllTestCaseFinished()
          .map((testCaseFinished) => query.findTestCaseBy(testCaseFinished))
          .map((testCase) => testCase?.id),
        testStepStarted: query
          .findAllTestStepStarted()
          .map((testStepStarted) => query.findTestCaseBy(testStepStarted))
          .map((testCase) => testCase?.id),
        testStepFinished: query
          .findAllTestStepFinished()
          .map((testStepFinished) => query.findTestCaseBy(testStepFinished))
          .map((testCase) => testCase?.id),
      }
    },
    findTestCaseDurationBy: (query: Query) => {
      return {
        testCaseStarted: query
          .findAllTestCaseStarted()
          .map((testCaseStarted) => query.findTestCaseDurationBy(testCaseStarted)),
        testCaseFinished: query
          .findAllTestCaseFinished()
          .map((testCaseFinished) => query.findTestCaseDurationBy(testCaseFinished)),
      }
    },
    findTestCaseFinishedBy: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .map((testCaseStarted) => query.findTestCaseFinishedBy(testCaseStarted))
        .map((testCaseFinished) => testCaseFinished?.testCaseStartedId),
    findTestRunDuration: (query: Query) => query.findTestRunDuration(),
    findTestRunFinished: (query: Query) => query.findTestRunFinished(),
    findTestRunStarted: (query: Query) => query.findTestRunStarted(),
    findTestStepByTestStepStarted: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .flatMap((testCaseStarted) => query.findTestStepsStartedBy(testCaseStarted))
        .map((testStepStarted) => query.findTestStepBy(testStepStarted))
        .map((testStep) => testStep?.id),
    findTestStepByTestStepFinished: (query: Query) => {
      return {
        testCaseStarted: query
          .findAllTestCaseStarted()
          .flatMap((testCaseStarted) => query.findTestStepsFinishedBy(testCaseStarted))
          .map((testStepFinished) => query.findTestStepBy(testStepFinished))
          .map((testStep) => testStep?.id),
        testCaseFinished: query
          .findAllTestCaseFinished()
          .flatMap((testCaseFinished) => query.findTestStepsFinishedBy(testCaseFinished))
          .map((testStepFinished) => query.findTestStepBy(testStepFinished))
          .map((testStep) => testStep?.id),
      }
    },
    findTestStepsFinishedBy: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .map((testCaseStarted) => query.findTestStepsFinishedBy(testCaseStarted))
        .map((testStepFinisheds) =>
          testStepFinisheds.map((testStepFinished) => testStepFinished?.testStepId)
        ),
    findTestStepFinishedAndTestStepBy: (query: Query) =>
      query
        .findAllTestCaseStarted()
        .flatMap((testCaseStarted) => query.findTestStepFinishedAndTestStepBy(testCaseStarted))
        .map(([testStepFinished, testStep]) => [testStepFinished.testStepId, testStep.id]),
  }

  for (const source of sources) {
    for (const methodName in queries) {
      const [suiteName] = path.basename(source).split('.')

      it(suiteName + ' -> ' + methodName, async () => {
        const query = new Query()

        await asyncPipeline(
          fs.createReadStream(source, { encoding: 'utf-8' }),
          new NdjsonToMessageStream(),
          new Writable({
            objectMode: true,
            write(envelope: Envelope, _: BufferEncoding, callback) {
              query.update(envelope)
              callback()
            },
          })
        )

        const expectedResults = JSON.parse(
          fs.readFileSync(
            path.join(
              __dirname,
              '../../testdata/' + suiteName + '.' + methodName + '.results.json'
            ),
            {
              encoding: 'utf-8',
            }
          )
        )
        const actualResults = JSON.parse(JSON.stringify(queries[methodName](query)))
        assert.deepStrictEqual(actualResults, expectedResults)
      })
    }
  }
})

type Queries = {
  // eslint-disable-next-line @typescript-eslint/no-wrapper-object-types
  [key: string]: (query: Query) => Object
}
