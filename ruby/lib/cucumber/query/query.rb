# frozen_string_literal: true

module Cucumber
  module Query
    class Query
      include Finders

      STATUS_ORDINAL = {
        'UNKNOWN' => 0,
        'PASSED' => 1,
        'SKIPPED' => 2,
        'PENDING' => 3,
        'UNDEFINED' => 4,
        'AMBIGUOUS' => 5,
        'FAILED' => 6
      }.freeze

      def initialize(state = State.new, indexer = EnvelopeIndexer.new(state))
        @state = state
        @indexer = indexer
      end

      def envelopes = @state.envelopes

      def update(envelope) = @indexer.update(envelope)

      def count_most_severe_test_step_result_status
        STATUS_ORDINAL.keys.to_h { |status| [status, count_status(status)] }
      end

      def count_test_cases_started = find_all_test_case_started.length

      def find_all_pickles = @state.pickle_by_id.values
      def find_all_pickle_steps = @state.pickle_step_by_id.values
      def find_all_step_definitions = @state.step_definition_by_id.values
      def find_all_test_cases = @state.test_case_by_id.values
      def find_all_test_steps = @state.test_step_by_id.values
      def find_all_test_step_started = @state.test_step_started_by_test_case_started_id.values.flatten
      def find_all_test_step_finished = @state.test_step_finished_by_test_case_started_id.values.flatten
      def find_all_test_run_hook_started = @state.test_run_hook_started_by_id.values
      def find_all_test_run_hook_finished = @state.test_run_hook_finished_by_test_run_hook_started_id.values
      def find_all_undefined_parameter_types = @state.undefined_parameter_types.dup
      def find_meta = @state.meta
      def find_test_run_finished = @state.test_run_finished
      def find_test_run_started = @state.test_run_started

      def find_all_test_case_started_order_by(find_order_by, order)
        ResultOrder.new(find_order_by, order).sort(self, find_all_test_case_started)
      end

      def find_all_test_case_finished_order_by(find_order_by, order)
        ResultOrder.new(find_order_by, order).sort(self, find_all_test_case_finished)
      end

      private

      def count_status(status)
        find_all_test_case_started.count do |test_case_started|
          find_most_severe_test_step_result_by(test_case_started)&.status == status
        end
      end

      def retried?(test_case_started)
        @state.test_case_finished_by_test_case_started_id[test_case_started.id]&.will_be_retried
      end

      def test_step_attachments(element)
        @state.attachments_by_test_case_started_id[element.test_case_started_id]
              .select { |attachment| attachment.test_step_id == element.test_step_id }
      end

      def test_step_results(test_case_started)
        find_test_step_finished_and_test_step_by(test_case_started)
          .map { |test_step_finished, _test_step| test_step_finished.test_step_result }
      end

      def timestamp_ms(timestamp)
        (timestamp.seconds * 1_000) + timestamp.nanos.fdiv(1_000_000)
      end

      def duration_from_ms(milliseconds)
        seconds = milliseconds.div(1_000)
        nanos = ((milliseconds - (seconds * 1_000)) * 1_000_000).round
        Cucumber::Messages::Duration.new(seconds: seconds, nanos: nanos)
      end

      def status_ordinal(status) = STATUS_ORDINAL.fetch(status)
    end
  end
end
