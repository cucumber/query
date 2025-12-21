import assert from 'node:assert'

import { TestCaseStarted } from '@cucumber/messages'

import Query from './Query'

describe('Query', () => {
  let cucumberQuery: Query
  beforeEach(() => {
    cucumberQuery = new Query()
  })

  describe('#findAllTestCaseStarted', () => {
    it('retains timestamp order', () => {
      const testCasesStarted: TestCaseStarted[] = [
        {
          id: '1',
          testCaseId: '1',
          attempt: 0,
          timestamp: {
            seconds: 1,
            nanos: 1,
          },
        },
        {
          id: '2',
          testCaseId: '2',
          attempt: 0,
          timestamp: {
            seconds: 2,
            nanos: 1,
          },
        },
        {
          id: '3',
          testCaseId: '3',
          attempt: 0,
          timestamp: {
            seconds: 2,
            nanos: 3,
          },
        },
      ]

      testCasesStarted
        .map((testCaseStarted) => ({ testCaseStarted }))
        .reverse()
        .forEach((envelope) => {
          cucumberQuery.update(envelope)
        })

      assert.deepStrictEqual(cucumberQuery.findAllTestCaseStarted(), testCasesStarted)
    })

    it('uses id as tie breaker', () => {
      const testCasesStarted: TestCaseStarted[] = [
        {
          id: '1',
          testCaseId: '1',
          attempt: 0,
          timestamp: {
            seconds: 1,
            nanos: 1,
          },
        },
        {
          id: '2',
          testCaseId: '2',
          attempt: 0,
          timestamp: {
            seconds: 1,
            nanos: 1,
          },
        },
      ]

      testCasesStarted
        .map((testCaseStarted) => ({ testCaseStarted }))
        .reverse()
        .forEach((envelope) => {
          cucumberQuery.update(envelope)
        })

      assert.deepStrictEqual(cucumberQuery.findAllTestCaseStarted(), testCasesStarted)
    })
  })
})
