# frozen_string_literal: true

module Cucumber
  module Query
    class EnvelopeIndexer
      HANDLERS = {
        meta: :update_meta,
        gherkin_document: :update_gherkin_document,
        pickle: :update_pickle,
        hook: :update_hook,
        step_definition: :update_step_definition,
        test_run_started: :update_test_run_started,
        test_run_hook_started: :update_test_run_hook_started,
        test_run_hook_finished: :update_test_run_hook_finished,
        test_case: :update_test_case,
        test_case_started: :update_test_case_started,
        test_step_started: :update_test_step_started,
        attachment: :update_attachment,
        test_step_finished: :update_test_step_finished,
        test_case_finished: :update_test_case_finished,
        test_run_finished: :update_test_run_finished,
        suggestion: :update_suggestion,
        undefined_parameter_type: :update_undefined_parameter_type
      }.freeze

      def initialize(state, gherkin_indexer = GherkinIndexer.new(state))
        @state = state
        @gherkin_indexer = gherkin_indexer
      end

      def update(envelope)
        @state.envelopes << envelope

        HANDLERS.each do |attribute, handler|
          value = envelope.public_send(attribute)
          public_send(handler, value) if value
        end
        nil
      end

      def update_meta(meta) = @state.meta = meta
      def update_gherkin_document(gherkin_document) = @gherkin_indexer.update(gherkin_document)
      def update_hook(hook) = @state.hook_by_id[hook.id] = hook
      def update_test_run_started(test_run_started) = @state.test_run_started = test_run_started
      def update_test_run_finished(test_run_finished) = @state.test_run_finished = test_run_finished

      def update_pickle(pickle)
        @state.pickle_by_id[pickle.id] = pickle
        pickle.steps.each { |pickle_step| @state.pickle_step_by_id[pickle_step.id] = pickle_step }
      end

      def update_step_definition(step_definition)
        @state.step_definition_by_id[step_definition.id] = step_definition
      end

      def update_test_run_hook_started(test_run_hook_started)
        @state.test_run_hook_started_by_id[test_run_hook_started.id] = test_run_hook_started
      end

      def update_test_run_hook_finished(test_run_hook_finished)
        @state.test_run_hook_finished_by_test_run_hook_started_id[
          test_run_hook_finished.test_run_hook_started_id
        ] = test_run_hook_finished
      end

      def update_test_case(test_case)
        @state.test_case_by_id[test_case.id] = test_case
        test_case.test_steps.each { |test_step| @state.test_step_by_id[test_step.id] = test_step }
      end

      def update_test_case_started(test_case_started)
        @state.test_case_started_by_id[test_case_started.id] = test_case_started
      end

      def update_test_step_started(test_step_started)
        @state.test_step_started_by_test_case_started_id[test_step_started.test_case_started_id] << test_step_started
      end

      def update_attachment(attachment)
        if attachment.test_case_started_id
          @state.attachments_by_test_case_started_id[attachment.test_case_started_id] << attachment
        end
        return unless attachment.test_run_hook_started_id

        @state.attachments_by_test_run_hook_started_id[attachment.test_run_hook_started_id] << attachment
      end

      def update_test_step_finished(test_step_finished)
        @state.test_step_finished_by_test_case_started_id[test_step_finished.test_case_started_id] << test_step_finished
      end

      def update_test_case_finished(test_case_finished)
        @state.test_case_finished_by_test_case_started_id[test_case_finished.test_case_started_id] = test_case_finished
      end

      def update_suggestion(suggestion)
        @state.suggestions_by_pickle_step_id[suggestion.pickle_step_id] << suggestion
      end

      def update_undefined_parameter_type(undefined_parameter_type)
        @state.undefined_parameter_types << undefined_parameter_type
      end
    end
  end
end
