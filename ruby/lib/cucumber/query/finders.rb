# frozen_string_literal: true

module Cucumber
  module Query
    module Finders
      def find_all_test_case_started
        @state.test_case_started_by_id.values
              .reject { |started| retried?(started) }
              .sort_by { |started| [timestamp_ms(started.timestamp), started.id] }
      end

      def find_all_test_case_finished
        @state.test_case_finished_by_test_case_started_id.values
              .reject(&:will_be_retried)
              .sort_by { |finished| [timestamp_ms(finished.timestamp), finished.test_case_started_id] }
      end

      def find_attachments_by(element)
        return test_step_attachments(element) if element.respond_to?(:test_step_id) && element.test_step_id

        @state.attachments_by_test_run_hook_started_id[element.test_run_hook_started_id]
      end

      def find_hook_by(item)
        return find_hook_by(find_test_run_hook_started_by(item)) if item.respond_to?(:test_run_hook_started_id)
        return nil unless item.respond_to?(:hook_id) && item.hook_id

        @state.hook_by_id[item.hook_id]
      end

      def find_most_severe_test_step_result_by(element)
        test_case_started = element.respond_to?(:test_case_started_id) ? find_test_case_started_by(element) : element
        test_step_results(test_case_started).max_by { |test_step_result| status_ordinal(test_step_result.status) }
      end

      def find_location_of(pickle) = pickle.location

      def find_pickle_by(element)
        test_case = find_test_case_by(element)
        test_case && @state.pickle_by_id[test_case.pickle_id]
      end

      def find_pickle_step_by(test_step)
        return nil unless test_step.respond_to?(:pickle_step_id) && test_step.pickle_step_id

        @state.pickle_step_by_id[test_step.pickle_step_id]
      end

      def find_step_by(pickle_step) = @state.step_by_id[pickle_step.ast_node_ids.first]

      def find_step_definitions_by(test_step)
        (test_step.step_definition_ids || []).filter_map { |id| @state.step_definition_by_id[id] }
      end

      def find_suggestions_by(element)
        return element.steps.flat_map { |step| find_suggestions_by(step) } if element.respond_to?(:steps)

        @state.suggestions_by_pickle_step_id[element.id]
      end

      def find_unambiguous_step_definition_by(test_step)
        return nil unless test_step.step_definition_ids&.length == 1

        @state.step_definition_by_id[test_step.step_definition_ids.first]
      end

      def find_test_case_by(element)
        test_case_started = element.respond_to?(:test_case_started_id) ? find_test_case_started_by(element) : element
        test_case_started && @state.test_case_by_id[test_case_started.test_case_id]
      end

      def find_test_case_duration_by(element)
        test_case_started = element.respond_to?(:test_case_started_id) ? find_test_case_started_by(element) : element
        test_case_finished = element.respond_to?(:test_case_started_id) ? element : find_test_case_finished_by(element)
        return nil unless test_case_started && test_case_finished

        duration_from_ms(timestamp_ms(test_case_finished.timestamp) - timestamp_ms(test_case_started.timestamp))
      end

      def find_test_case_started_by(element) = @state.test_case_started_by_id[element.test_case_started_id]

      def find_test_case_finished_by(test_case_started)
        @state.test_case_finished_by_test_case_started_id[test_case_started.id]
      end

      def find_test_run_hook_started_by(test_run_hook_finished)
        @state.test_run_hook_started_by_id[test_run_hook_finished.test_run_hook_started_id]
      end

      def find_test_run_hook_finished_by(test_run_hook_started)
        @state.test_run_hook_finished_by_test_run_hook_started_id[test_run_hook_started.id]
      end

      def find_test_step_by(element) = @state.test_step_by_id[element.test_step_id]

      def find_test_run_duration
        return nil unless @state.test_run_started && @state.test_run_finished

        started_at = timestamp_ms(@state.test_run_started.timestamp)
        finished_at = timestamp_ms(@state.test_run_finished.timestamp)
        duration_from_ms(finished_at - started_at)
      end

      def find_test_steps_started_by(element)
        test_case_started_id = element.respond_to?(:test_case_started_id) ? element.test_case_started_id : element.id
        @state.test_step_started_by_test_case_started_id[test_case_started_id]
      end

      def find_test_steps_finished_by(element)
        test_case_started = element.respond_to?(:test_case_started_id) ? find_test_case_started_by(element) : element
        @state.test_step_finished_by_test_case_started_id[test_case_started.id]
      end

      def find_test_step_finished_and_test_step_by(test_case_started)
        @state.test_step_finished_by_test_case_started_id[test_case_started.id].map do |test_step_finished|
          [test_step_finished, find_test_step_by(test_step_finished)]
        end
      end

      def find_lineage_by(element)
        pickle = element.respond_to?(:ast_node_ids) ? element : find_pickle_by(element)
        return nil unless pickle

        ast_node_id = pickle.ast_node_ids&.last
        return nil unless ast_node_id

        @state.lineage_by_id[ast_node_id]
      end
    end
  end
end
