# frozen_string_literal: true

# rubocop:disable RSpec/DescribeClass, Lint/ConstantDefinitionInBlock, RSpec/LeakyConstantDeclaration

require 'cucumber/messages/helpers/ndjson_to_message_enumerator'

RSpec.describe 'shared cucumber-query testdata' do
  TESTDATA_DIR = File.expand_path('../../../testdata/src', __dir__)

  def load_query(suite)
    query = Cucumber::Query::Query.new
    File.open(File.join(TESTDATA_DIR, "#{suite}.ndjson"), 'r') do |io|
      Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(io).each do |envelope|
        query.update(envelope)
      end
    end
    query
  end

  def json(value)
    JSON.parse(JSON.generate(value))
  end

  EXAMPLE_NAME = lambda do |lineage|
    "##{lineage[:examples_index] + 1}.#{lineage[:example_index] + 1}"
  end

  LINEAGE_NAME = lambda do |lineage|
    parts = []
    parts << lineage[:feature]&.name
    parts << lineage[:rule]&.name
    parts << lineage[:scenario]&.name
    parts << lineage[:examples]&.name
    parts << EXAMPLE_NAME.call(lineage) if lineage[:example]
    parts.compact.reject(&:empty?).join(' - ')
  end

  REVERSE_PICKLE_COMPARATOR = lambda do |left, right|
    [right.uri, right.location.line, right.location.column] <=> [left.uri, left.location.line, left.location.column]
  end

  collection_cases = {
    'findAllTestCaseStartedOrderBy' => lambda { |query|
      query.find_all_test_case_started_order_by(
        ->(candidate_query, test_case_started) { candidate_query.find_pickle_by(test_case_started) },
        REVERSE_PICKLE_COMPARATOR
      ).map(&:id)
    },
    'findAllTestCaseFinishedOrderBy' => lambda { |query|
      query.find_all_test_case_finished_order_by(
        ->(candidate_query, test_case_finished) { candidate_query.find_pickle_by(test_case_finished) },
        REVERSE_PICKLE_COMPARATOR
      ).map(&:test_case_started_id)
    },
    'findAllTestCases' => ->(query) { query.find_all_test_cases.length },
    'findAllTestStepsStarted' => ->(query) { query.find_all_test_step_started.length },
    'findAllTestStepsFinished' => ->(query) { query.find_all_test_step_finished.length },
    'findAllUndefinedParameterTypes' => lambda { |query|
      query.find_all_undefined_parameter_types.map do |undefined_parameter_type|
        [undefined_parameter_type.name, undefined_parameter_type.expression]
      end
    },
    'findTestRunStarted' => lambda(&:find_test_run_started),
    'findTestRunFinished' => lambda(&:find_test_run_finished)
  }

  relationship_cases = {
    'findLineageBy' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.filter_map do |item|
          lineage = query.find_lineage_by(item)
          LINEAGE_NAME.call(lineage) if lineage
        end,
        testCaseFinished: query.find_all_test_case_finished.filter_map do |item|
          lineage = query.find_lineage_by(item)
          LINEAGE_NAME.call(lineage) if lineage
        end,
        pickle: query.find_all_pickles.filter_map do |item|
          lineage = query.find_lineage_by(item)
          LINEAGE_NAME.call(lineage) if lineage
        end
      }
    },
    'findLocationOf' => ->(query) { query.find_all_pickles.filter_map { |item| query.find_location_of(item) } },
    'findPickleBy' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.map { |item| query.find_pickle_by(item)&.name },
        testCaseFinished: query.find_all_test_case_finished.map { |item| query.find_pickle_by(item)&.name },
        testStepStarted: query.find_all_test_step_started.map { |item| query.find_pickle_by(item)&.name },
        testStepFinished: query.find_all_test_step_finished.map { |item| query.find_pickle_by(item)&.name }
      }
    },
    'findPickleStepBy' => lambda { |query|
      query.find_all_test_steps.filter_map { |item| query.find_pickle_step_by(item)&.text }
    },
    'findStepBy' => ->(query) { query.find_all_pickle_steps.map { |item| query.find_step_by(item)&.text } },
    'findStepDefinitionsBy' => lambda { |query|
      query.find_all_test_steps.map { |item| query.find_step_definitions_by(item).map(&:id) }
    },
    'findSuggestionsBy' => lambda { |query|
      {
        pickleStep: query.find_all_pickle_steps.flat_map { |item| query.find_suggestions_by(item) }.map(&:id),
        pickle: query.find_all_pickles.flat_map { |item| query.find_suggestions_by(item) }.map(&:id)
      }
    },
    'findUnambiguousStepDefinitionBy' => lambda { |query|
      query.find_all_test_steps.filter_map { |item| query.find_unambiguous_step_definition_by(item)&.id }
    },
    'findTestCaseStartedBy' => lambda { |query|
      {
        testCaseFinished: query.find_all_test_case_finished.map { |item| query.find_test_case_started_by(item)&.id },
        testStepStarted: query.find_all_test_step_started.map { |item| query.find_test_case_started_by(item)&.id },
        testStepFinished: query.find_all_test_step_finished.map { |item| query.find_test_case_started_by(item)&.id }
      }
    },
    'findTestCaseBy' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.map { |item| query.find_test_case_by(item)&.id },
        testCaseFinished: query.find_all_test_case_finished.map { |item| query.find_test_case_by(item)&.id },
        testStepStarted: query.find_all_test_step_started.map { |item| query.find_test_case_by(item)&.id },
        testStepFinished: query.find_all_test_step_finished.map { |item| query.find_test_case_by(item)&.id }
      }
    },
    'findTestCaseFinishedBy' => lambda { |query|
      query.find_all_test_case_started.map { |item| query.find_test_case_finished_by(item)&.test_case_started_id }
    },
    'findTestStepBy' => lambda { |query|
      query.find_all_test_case_started.flat_map { |item| query.find_test_steps_started_by(item) }
           .map { |item| query.find_test_step_by(item)&.id }
    },
    'findTestStepsStartedBy' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.map do |item|
          query.find_test_steps_started_by(item).map(&:test_step_id)
        end,
        testCaseFinished: query.find_all_test_case_finished.map do |item|
          query.find_test_steps_started_by(item).map(&:test_step_id)
        end
      }
    },
    'findTestStepsFinishedBy' => lambda { |query|
      query.find_all_test_case_started.map { |item| query.find_test_steps_finished_by(item).map(&:test_step_id) }
    },
    'findTestStepByTestStepFinished' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.flat_map { |item| query.find_test_steps_finished_by(item) }
                              .map { |item| query.find_test_step_by(item)&.id },
        testCaseFinished: query.find_all_test_case_finished.flat_map { |item| query.find_test_steps_finished_by(item) }
                               .map { |item| query.find_test_step_by(item)&.id }
      }
    },
    'findTestStepFinishedAndTestStepBy' => lambda { |query|
      query.find_all_test_case_started.flat_map { |item| query.find_test_step_finished_and_test_step_by(item) }
           .map { |test_step_finished, test_step| [test_step_finished.test_step_id, test_step.id] }
    }
  }

  attachment_and_severity_cases = {
    'findAttachmentsBy' => lambda { |query|
      test_step_attachments = query.find_all_test_case_started
                                   .flat_map { |started| query.find_test_steps_finished_by(started) }
                                   .flat_map { |finished| query.find_attachments_by(finished) }
      test_run_hook_attachments = query.find_all_test_run_hook_finished
                                       .flat_map { |finished| query.find_attachments_by(finished) }

      {
        testStepFinished: test_step_attachments.map do |attachment|
          [
            attachment.test_step_id,
            attachment.test_case_started_id,
            attachment.media_type,
            attachment.content_encoding
          ]
        end,
        testRunHookFinished: test_run_hook_attachments.map do |attachment|
          [
            attachment.test_run_hook_started_id,
            attachment.media_type,
            attachment.content_encoding
          ]
        end
      }
    },
    'findMostSevereTestStepResultBy' => lambda { |query|
      {
        testCaseStarted: query.find_all_test_case_started.filter_map do |test_case_started|
          query.find_most_severe_test_step_result_by(test_case_started)&.status
        end,
        testCaseFinished: query.find_all_test_case_finished.filter_map do |test_case_finished|
          query.find_most_severe_test_step_result_by(test_case_finished)&.status
        end
      }
    }
  }

  cases = {
    'minimal' => {
      'countMostSevereTestStepResultStatus' => lambda(&:count_most_severe_test_step_result_status),
      'countTestCasesStarted' => lambda(&:count_test_cases_started),
      'findAllPickles' => ->(query) { query.find_all_pickles.length },
      'findAllPickleSteps' => ->(query) { query.find_all_pickle_steps.length },
      'findAllStepDefinitions' => ->(query) { query.find_all_step_definitions.length },
      'findAllTestCaseStarted' => ->(query) { query.find_all_test_case_started.length },
      'findAllTestCaseFinished' => ->(query) { query.find_all_test_case_finished.length },
      'findAllTestSteps' => ->(query) { query.find_all_test_steps.length },
      'findMeta' => ->(query) { query.find_meta&.implementation&.name },
      'findTestCaseDurationBy' => lambda { |query|
        {
          testCaseStarted: query.find_all_test_case_started.map { |item| query.find_test_case_duration_by(item)&.to_h },
          testCaseFinished: query.find_all_test_case_finished.map do |item|
            query.find_test_case_duration_by(item)&.to_h
          end
        }
      },
      'findTestRunDuration' => ->(query) { query.find_test_run_duration&.to_h }
    },
    'hooks' => {
      'findAllTestCaseStarted' => ->(query) { query.find_all_test_case_started.length },
      'findAllTestSteps' => ->(query) { query.find_all_test_steps.length },
      'findHookBy' => lambda { |query|
        {
          testStep: query.find_all_test_steps.filter_map { |item| query.find_hook_by(item)&.id },
          testRunHookStarted: query.find_all_test_run_hook_started.filter_map { |item| query.find_hook_by(item)&.id },
          testRunHookFinished: query.find_all_test_run_hook_finished.filter_map { |item| query.find_hook_by(item)&.id }
        }
      }
    },
    'global-hooks' => {
      'findAllTestRunHookStarted' => ->(query) { query.find_all_test_run_hook_started.length },
      'findAllTestRunHookFinished' => ->(query) { query.find_all_test_run_hook_finished.length },
      'findTestRunHookStartedBy' => lambda { |query|
        query.find_all_test_run_hook_finished.map { |item| query.find_test_run_hook_started_by(item)&.id }
      },
      'findTestRunHookFinishedBy' => lambda { |query|
        query.find_all_test_run_hook_started.map do |item|
          query.find_test_run_hook_finished_by(item)&.test_run_hook_started_id
        end
      }
    }
  }

  %w[
    attachments
    empty
    global-hooks
    global-hooks-attachments
    hooks
    minimal
    rules
    examples-tables
    unknown-parameter-type
  ].each do |suite|
    cases[suite] ||= {}
    cases[suite].merge!(collection_cases, relationship_cases, attachment_and_severity_cases)
  end

  naming_cases = {
    'long' => Cucumber::Query.naming_strategy(Cucumber::Query::NAMING_STRATEGY_LENGTH_LONG),
    'short' => Cucumber::Query.naming_strategy(Cucumber::Query::NAMING_STRATEGY_LENGTH_SHORT),
    'long-exclude-feature-name' => Cucumber::Query.naming_strategy(
      Cucumber::Query::NAMING_STRATEGY_LENGTH_LONG,
      Cucumber::Query::NAMING_STRATEGY_FEATURE_NAME_EXCLUDE
    ),
    'long-with-pickle-name' => Cucumber::Query.naming_strategy(
      Cucumber::Query::NAMING_STRATEGY_LENGTH_LONG,
      Cucumber::Query::NAMING_STRATEGY_FEATURE_NAME_INCLUDE,
      Cucumber::Query::NAMING_STRATEGY_EXAMPLE_NAME_PICKLE
    ),
    'long-with-pickle-name-if-parameterized' => Cucumber::Query.naming_strategy(
      Cucumber::Query::NAMING_STRATEGY_LENGTH_LONG,
      Cucumber::Query::NAMING_STRATEGY_FEATURE_NAME_INCLUDE,
      Cucumber::Query::NAMING_STRATEGY_EXAMPLE_NAME_NUMBER_AND_PICKLE_IF_PARAMETERIZED
    )
  }

  cases.each do |suite, suite_cases|
    suite_cases.each do |query_name, operation|
      it "matches #{suite}.#{query_name}.results.json" do
        expected = JSON.parse(File.read(File.join(TESTDATA_DIR, "#{suite}.#{query_name}.results.json")))
        expect(json(operation.call(load_query(suite)))).to eq(expected)
      end
    end
  end

  %w[minimal rules examples-tables].each do |suite|
    naming_cases.each do |strategy_name, strategy|
      it "matches #{suite}.naming-strategy.#{strategy_name}.txt" do
        query = load_query(suite)
        names = query.find_all_pickles.map { |pickle| strategy.reduce(query.find_lineage_by(pickle), pickle) }
        expected = naming_expectation(suite, strategy_name)

        expect(names).to eq(expected)
      end
    end
  end

  def naming_expectation(suite, strategy_name)
    File.readlines(File.join(TESTDATA_DIR, "#{suite}.naming-strategy.#{strategy_name}.txt"), chomp: true)
  end
end

# rubocop:enable RSpec/DescribeClass, Lint/ConstantDefinitionInBlock, RSpec/LeakyConstantDeclaration
