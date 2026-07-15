# frozen_string_literal: true

require 'cucumber/messages'

require_relative 'repository'

# Given one Cucumber Message, find another.
#
# Queries can be made while the test run is incomplete - and this will naturally return incomplete results
# see {Cucumber Messages - Message Overview}[https://github.com/cucumber/messages?tab=readme-ov-file#message-overview]
#
module Cucumber
  # Provides lookup methods for related Cucumber messages stored in the `Repository` class.
  class Query
    attr_reader :repository
    private :repository

    include Cucumber::Messages::Helpers::TimeConversion
    include Cucumber::Messages::Helpers::TestStepResultComparator

    def initialize(repository)
      @repository = repository
    end

    def count_test_cases_started
      find_all_test_case_started.length
    end

    def find_all_pickles
      repository.pickle_by_id.values
    end

    def find_all_pickle_steps
      repository.pickle_step_by_id.values
    end

    def find_all_step_definitions
      repository.step_definition_by_id.values
    end

    # This finds all test cases from the following conditions (UNION)
    #   -> Test cases that have started, but not yet finished
    #   -> Test cases that have started, finished, but that will NOT be retried
    def find_all_test_case_started
      repository.test_case_started_by_id.values.select do |test_case_started|
        test_case_finished = find_test_case_finished_by(test_case_started)
        test_case_finished.nil? || !test_case_finished.will_be_retried
      end
    end

    # This finds all test cases that have finished AND will not be retried
    def find_all_test_case_finished
      repository.test_case_finished_by_test_case_started_id.values.reject(&:will_be_retried)
    end

    def find_all_test_cases
      repository.test_case_by_id.values
    end

    def find_all_test_run_hook_started
      repository.test_run_hook_started_by_id.values
    end

    def find_all_test_run_hook_finished
      repository.test_run_hook_finished_by_test_run_hook_started_id.values
    end

    def find_all_test_step_started
      repository.test_steps_started_by_test_case_started_id.values.flatten
    end

    def find_all_test_step_finished
      repository.test_steps_finished_by_test_case_started_id.values.flatten
    end

    def find_all_test_steps
      repository.test_step_by_id.values
    end

    # This method will be called with 1 of these 3 messages
    #   [TestStep || TestRunHookStarted || TestRunHookFinished]
    def find_hook_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestStep, Cucumber::Messages::TestRunHookStarted, Cucumber::Messages::TestRunHookFinished],
        '#find_hook_by'
      )

      if message.is_a?(Cucumber::Messages::TestRunHookFinished)
        test_run_hook_started_message = find_test_run_hook_started_by(message)
        test_run_hook_started_message ? find_hook_by(test_run_hook_started_message) : nil
      else
        repository.hook_by_id[message.hook_id]
      end
    end

    def find_meta
      repository.meta
    end

    # This method will be called with 1 of these 2 messages
    #   [TestCaseStarted || TestCaseFinished]
    def find_most_severe_test_step_result_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestCaseStarted, Cucumber::Messages::TestCaseFinished],
        '#find_most_severe_test_step_result_by'
      )

      if message.is_a?(Cucumber::Messages::TestCaseStarted)
        find_test_steps_finished_by(message)
          .map(&:test_step_result)
          .max_by { |test_step_result| test_step_result_rankings[test_step_result.status] }
      else
        test_case_started_message = find_test_case_started_by(message)
        test_case_started_message && find_most_severe_test_step_result_by(test_case_started_message)
      end
    end

    # This method will be called with 1 of these 5 messages
    #   [TestCase || TestCaseStarted || TestCaseFinished || TestStepStarted || TestStepFinished]
    def find_pickle_by(message)
      ensure_only_message_types!(
        message,
        [
          Cucumber::Messages::TestCase,
          Cucumber::Messages::TestCaseStarted,
          Cucumber::Messages::TestCaseFinished,
          Cucumber::Messages::TestStepStarted,
          Cucumber::Messages::TestStepFinished
        ],
        '#find_pickle_by'
      )

      test_case_message = message.is_a?(Cucumber::Messages::TestCase) ? message : find_test_case_by(message)
      repository.pickle_by_id[test_case_message.pickle_id]
    end

    # This method will be called with only 1 message
    #   [TestStep]
    def find_pickle_step_by(test_step)
      ensure_only_message_types!(
        test_step,
        [Cucumber::Messages::TestStep],
        '#find_pickle_step_by'
      )

      repository.pickle_step_by_id[test_step.pickle_step_id]
    end

    # This method will be called with only 1 message
    #   [PickleStep]
    def find_step_by(pickle_step)
      ensure_only_message_types!(
        pickle_step,
        [Cucumber::Messages::PickleStep],
        '#find_step_by'
      )

      repository.step_by_id[pickle_step.ast_node_ids.first]
    end

    # This method will be called with only 1 message
    #   [TestStep]
    def find_step_definitions_by(test_step)
      ensure_only_message_types!(
        test_step,
        [Cucumber::Messages::TestStep],
        '#find_step_definitions_by'
      )

      ids = test_step.step_definition_ids.nil? ? [] : test_step.step_definition_ids
      ids.filter_map { |id| repository.step_definition_by_id[id] }
    end

    # This method will be called with 1 of these 4 messages
    #   [TestCaseStarted || TestCaseFinished || TestStepStarted || TestStepFinished]
    def find_test_case_by(message)
      ensure_only_message_types!(
        message,
        [
          Cucumber::Messages::TestCaseStarted,
          Cucumber::Messages::TestCaseFinished,
          Cucumber::Messages::TestStepStarted,
          Cucumber::Messages::TestStepFinished
        ],
        '#find_test_case_by'
      )

      test_case_started_message = message.is_a?(Cucumber::Messages::TestCaseStarted) ? message : find_test_case_started_by(message)
      repository.test_case_by_id[test_case_started_message.test_case_id]
    end

    # This method will be called with 1 of these 3 messages
    #   [TestCaseFinished || TestStepStarted || TestStepFinished]
    def find_test_case_started_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestCaseFinished, Cucumber::Messages::TestStepStarted, Cucumber::Messages::TestStepFinished],
        '#find_test_case_started_by'
      )

      repository.test_case_started_by_id[message.test_case_started_id]
    end

    # This method will be called with only 1 message
    #   [TestCaseStarted]
    def find_test_case_finished_by(test_case_started)
      ensure_only_message_types!(
        test_case_started,
        [Cucumber::Messages::TestCaseStarted],
        '#find_test_case_finished_by'
      )

      repository.test_case_finished_by_test_case_started_id[test_case_started.id]
    end

    def find_test_run_duration
      if repository.test_run_started.nil? || repository.test_run_finished.nil?
        nil
      else
        float_time = timestamp_to_time(repository.test_run_finished.timestamp) - timestamp_to_time(repository.test_run_started.timestamp)
        seconds_to_duration(float_time)
      end
    end

    # This method will be called with only 1 message
    #   [TestRunHookFinished]
    def find_test_run_hook_started_by(test_run_hook_finished)
      ensure_only_message_types!(
        test_run_hook_finished,
        [Cucumber::Messages::TestRunHookFinished],
        '#find_test_run_hook_started_by'
      )

      repository.test_run_hook_started_by_id[test_run_hook_finished.test_run_hook_started_id]
    end

    # This method will be called with only 1 message
    #   [TestRunHookStarted]
    def find_test_run_hook_finished_by(test_run_hook_started)
      ensure_only_message_types!(
        test_run_hook_started,
        [Cucumber::Messages::TestRunHookStarted],
        '#find_test_run_hook_finished_by'
      )

      repository.test_run_hook_finished_by_test_run_hook_started_id[test_run_hook_started.id]
    end

    def find_test_run_started
      repository.test_run_started
    end

    def find_test_run_finished
      repository.test_run_finished
    end

    # This method will be called with 1 of these 2 messages
    #   [TestStepStarted || TestStepFinished]
    def find_test_step_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestStepStarted, Cucumber::Messages::TestStepFinished],
        '#find_test_step_by'
      )

      repository.test_step_by_id[message.test_step_id]
    end

    # This method will be called with 1 of these 2 messages
    #   [TestCaseStarted || TestCaseFinished]
    def find_test_steps_started_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestCaseStarted, Cucumber::Messages::TestCaseFinished],
        '#find_test_steps_started_by'
      )

      key = message.is_a?(Cucumber::Messages::TestCaseStarted) ? message.id : message.test_case_started_id
      repository.test_steps_started_by_test_case_started_id.fetch(key, [])
    end

    # This method will be called with 1 of these 2 messages
    #   [TestCaseStarted || TestCaseFinished]
    def find_test_steps_finished_by(message)
      ensure_only_message_types!(
        message,
        [Cucumber::Messages::TestCaseStarted, Cucumber::Messages::TestCaseFinished],
        '#find_test_steps_finished_by'
      )

      if message.is_a?(Cucumber::Messages::TestCaseStarted)
        repository.test_steps_finished_by_test_case_started_id.fetch(message.id, [])
      else
        test_case_started_message = find_test_case_started_by(message)
        test_case_started_message.nil? ? [] : find_test_steps_finished_by(test_case_started_message)
      end
    end

    private

    def ensure_only_message_types!(supplied_message, permissible_message_types, method_name)
      raise ArgumentError, "Supplied argument is not a Cucumber Message. Argument: #{supplied_message.class}" unless supplied_message.is_a?(Cucumber::Messages::Message)
      raise ArgumentError, "Supplied message type '#{supplied_message.class}' is not permitted to be used when calling #{method_name}" unless permissible_message_types.include?(supplied_message.class)
    end
  end
end
