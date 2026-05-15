# frozen_string_literal: true

module Cucumber
  module Query
    class State
      attr_accessor :meta, :test_run_started, :test_run_finished
      attr_reader :envelopes, :test_case_started_by_id, :lineage_by_id, :step_by_id,
                  :pickle_by_id, :pickle_step_by_id, :hook_by_id, :step_definition_by_id,
                  :test_case_by_id, :test_step_by_id, :test_case_finished_by_test_case_started_id,
                  :test_run_hook_started_by_id,
                  :test_run_hook_finished_by_test_run_hook_started_id,
                  :test_step_started_by_test_case_started_id,
                  :test_step_finished_by_test_case_started_id,
                  :attachments_by_test_case_started_id,
                  :attachments_by_test_run_hook_started_id,
                  :suggestions_by_pickle_step_id, :undefined_parameter_types

      def initialize
        @envelopes = []
        initialize_identity_indexes
        initialize_collection_indexes
        @undefined_parameter_types = []
      end

      private

      def initialize_identity_indexes
        identity_index_names.each { |name| instance_variable_set("@#{name}", {}) }
      end

      def identity_index_names
        %i[
          test_case_started_by_id lineage_by_id step_by_id pickle_by_id pickle_step_by_id
          hook_by_id step_definition_by_id test_case_by_id test_step_by_id
          test_case_finished_by_test_case_started_id test_run_hook_started_by_id
          test_run_hook_finished_by_test_run_hook_started_id
        ]
      end

      def initialize_collection_indexes
        @test_step_started_by_test_case_started_id = hash_of_arrays
        @test_step_finished_by_test_case_started_id = hash_of_arrays
        @attachments_by_test_case_started_id = hash_of_arrays
        @attachments_by_test_run_hook_started_id = hash_of_arrays
        @suggestions_by_pickle_step_id = hash_of_arrays
      end

      def hash_of_arrays
        Hash.new { |hash, key| hash[key] = [] }
      end
    end
  end
end
