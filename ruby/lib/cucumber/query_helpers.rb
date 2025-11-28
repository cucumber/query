require 'cucumber/messages/test_step_result_status'

module Cucumber
  module Query
    module QueryHelpers
      def status_ordinal(status)
        [
          TestStepResultStatus::UNKNOWN,
          TestStepResultStatus::PASSED,
          TestStepResultStatus::SKIPPED,
          TestStepResultStatus::PENDING,
          TestStepResultStatus::UNDEFINED,
          TestStepResultStatus::AMBIGUOUS,
          TestStepResultStatus::FAILED
        ].index(status)
      end

      def assert(target, failure_message)
        raise StandardError, failure_message unless target
      end
    end
  end
end
