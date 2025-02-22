import assert from 'node:assert'

import { TestStepResult, TestStepResultStatus } from '@cucumber/messages'

import { comparatorByStatus } from './helpers'

describe('comparatorByStatus', () => {
  function resultWithStatus(status: TestStepResultStatus) {
    return { status } as TestStepResult
  }

  it('puts the more severe status after the less severe', () => {
    assert.strictEqual(
      comparatorByStatus(
        resultWithStatus(TestStepResultStatus.PASSED),
        resultWithStatus(TestStepResultStatus.FAILED)
      ),
      -1
    )
    assert.strictEqual(
      comparatorByStatus(
        resultWithStatus(TestStepResultStatus.FAILED),
        resultWithStatus(TestStepResultStatus.PASSED)
      ),
      1
    )
    assert.strictEqual(
      comparatorByStatus(
        resultWithStatus(TestStepResultStatus.FAILED),
        resultWithStatus(TestStepResultStatus.FAILED)
      ),
      0
    )
  })
})
