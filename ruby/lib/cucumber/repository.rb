# frozen_string_literal: true

module Cucumber
  # In memory repository i.e. a thread based link to cucumber-query
  class Repository
    attr_accessor :meta, :test_run_started, :test_run_finished
    attr_reader :attachments_by_test_case_started_id, :attachments_by_test_run_hook_started_id,
                :hook_by_id,
                :pickle_by_id, :pickle_step_by_id,
                :suggestions_by_pickle_step_id,
                :step_by_id, :step_definition_by_id,
                :test_case_by_id, :test_case_started_by_id, :test_case_finished_by_test_case_started_id,
                :test_run_hook_started_by_id, :test_run_hook_finished_by_test_run_hook_started_id,
                :test_step_by_id, :test_steps_started_by_test_case_started_id, :test_steps_finished_by_test_case_started_id,
                :undefined_parameter_types

    def initialize
      @attachments_by_test_case_started_id = _hash_with_array_default
      @attachments_by_test_run_hook_started_id = _hash_with_array_default
      @hook_by_id = {}
      @pickle_by_id = {}
      @pickle_step_by_id = {}
      @step_by_id = {}
      @step_definition_by_id = {}
      @suggestions_by_pickle_step_id = _hash_with_array_default
      @test_case_by_id = {}
      @test_case_started_by_id = {}
      @test_case_finished_by_test_case_started_id = {}
      @test_run_hook_started_by_id = {}
      @test_run_hook_finished_by_test_run_hook_started_id = {}
      @test_step_by_id = {}
      @test_steps_started_by_test_case_started_id = _hash_with_array_default
      @test_steps_finished_by_test_case_started_id = _hash_with_array_default
      @undefined_parameter_types = []
    end

    def update(envelope)
      message_category = envelope.type[:contained_message]
      message_value = envelope.public_send(message_category)
      send("update_#{message_category}", message_value)
    end

    private

    def method_missing(method_name, *args, &)
      if method_name.to_s.start_with?('update_')
        Kernel.warn("Attempting to update the repository with #{method_name}. Please raise this as a missing message handler")
        Kernel.warn('Create an issue here: https://github.com/cucumber/query/issues')
        nil
      else
        super
      end
    end

    def respond_to_missing?(method_name, include_private = false)
      method_name.to_s.start_with?('update_') || super
    end

    def update_attachment(attachment)
      attachments_by_test_case_started_id[attachment.test_case_started_id] << attachment if attachment.test_case_started_id
      attachments_by_test_run_hook_started_id[attachment.test_run_hook_started_id] << attachment if attachment.test_run_hook_started_id
    end

    def update_feature(feature)
      feature.children.each do |feature_child|
        update_steps(feature_child.background.steps) if feature_child.background
        update_scenario(feature_child.scenario) if feature_child.scenario
        feature_child.rule&.children&.each { |rule_child| _update_feature_rule(rule_child) }
      end
    end

    def update_gherkin_document(gherkin_document)
      update_feature(gherkin_document.feature) if gherkin_document.feature
    end

    def update_hook(hook)
      hook_by_id[hook.id] = hook
    end

    def update_meta(meta)
      self.meta = meta
    end

    def update_pickle(pickle)
      pickle_by_id[pickle.id] = pickle
      pickle.steps.each { |pickle_step| pickle_step_by_id[pickle_step.id] = pickle_step }
    end

    def update_scenario(scenario)
      update_steps(scenario.steps)
    end

    def update_source(_source)
      # This deliberately doesn't perform any handling. `Source` as a message is not stored or required
      #   - See `GherkinDocument` for a more "parsed" form of an AST representation
      :no_op
    end

    def update_steps(steps)
      steps.each { |step| step_by_id[step.id] = step }
    end

    def update_step_definition(step_definition)
      step_definition_by_id[step_definition.id] = step_definition
    end

    def update_suggestion(suggestion)
      suggestions_by_pickle_step_id[suggestion.pickle_step_id] << suggestion
    end

    def update_test_case(test_case)
      test_case_by_id[test_case.id] = test_case
      test_case.test_steps.each { |test_step| test_step_by_id[test_step.id] = test_step }
    end

    def update_test_case_started(test_case_started)
      test_case_started_by_id[test_case_started.id] = test_case_started
    end

    def update_test_case_finished(test_case_finished)
      test_case_finished_by_test_case_started_id[test_case_finished.test_case_started_id] = test_case_finished
    end

    def update_test_run_started(test_run_started)
      self.test_run_started = test_run_started
    end

    def update_test_run_finished(test_run_finished)
      self.test_run_finished = test_run_finished
    end

    def update_test_run_hook_started(test_run_hook_started)
      test_run_hook_started_by_id[test_run_hook_started.id] = test_run_hook_started
    end

    def update_test_run_hook_finished(test_run_hook_finished)
      test_run_hook_finished_by_test_run_hook_started_id[test_run_hook_finished.test_run_hook_started_id] = test_run_hook_finished
    end

    def update_test_step_started(test_step_started)
      test_steps_started_by_test_case_started_id[test_step_started.test_case_started_id] << test_step_started
    end

    def update_test_step_finished(test_step_finished)
      test_steps_finished_by_test_case_started_id[test_step_finished.test_case_started_id] << test_step_finished
    end

    def update_undefined_parameter_type(undefined_parameter_type)
      undefined_parameter_types << undefined_parameter_type
    end

    def _update_feature_rule(rule_child)
      update_steps(rule_child.background.steps) if rule_child.background
      update_scenario(rule_child.scenario) if rule_child.scenario
    end

    def _hash_with_array_default
      Hash.new { |hash, key| hash[key] = [] }
    end
  end
end
