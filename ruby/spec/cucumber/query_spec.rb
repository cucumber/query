# frozen_string_literal: true

RSpec.describe Cucumber::Query do
  def testdata_path(name)
    File.expand_path("../../../testdata/src/#{name}", __dir__)
  end

  def parse_ndjson_file(path)
    Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(File.read(path))
  end

  queries = {
    'countTestCasesStarted' => lambda(&:count_test_cases_started),
    'findAllPickles' => ->(query) { query.find_all_pickles.length },
    'findAllPickleSteps' => ->(query) { query.find_all_pickle_steps.length },
    'findAllStepDefinitions' => ->(query) { query.find_all_step_definitions.length },
    'findAllTestCaseStarted' => ->(query) { query.find_all_test_case_started.length },
    'findAllTestCaseFinished' => ->(query) { query.find_all_test_case_finished.length },
    'findAllTestCases' => ->(query) { query.find_all_test_cases.length },
    'findAllTestRunHookStarted' => ->(query) { query.find_all_test_run_hook_started.length },
    'findAllTestRunHookFinished' => ->(query) { query.find_all_test_run_hook_finished.length },
    'findAllTestStepStarted' => ->(query) { query.find_all_test_step_started.length },
    'findAllTestStepFinished' => ->(query) { query.find_all_test_step_finished.length },
    'findAllTestSteps' => ->(query) { query.find_all_test_steps.length },
    'findHookBy' => lambda do |query|
      results = {}
      results['testStep'] = query.find_all_test_steps.filter_map { |message| query.find_hook_by(message)&.id }
      results['testRunHookStarted'] = query.find_all_test_run_hook_started.filter_map { |message| query.find_hook_by(message)&.id }
      results['testRunHookFinished'] = query.find_all_test_run_hook_finished.filter_map { |message| query.find_hook_by(message)&.id }
      results
    end,
    'findMeta' => ->(query) { query.find_meta.implementation.name },
    'findMostSevereTestStepResultBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.filter_map { |message| query.find_most_severe_test_step_result_by(message)&.status }
      results['testCaseFinished'] = query.find_all_test_case_finished.filter_map { |message| query.find_most_severe_test_step_result_by(message)&.status }
      results
    end,
    'findPickleBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_pickle_by(message).name }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_pickle_by(message).name }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_pickle_by(message).name }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_pickle_by(message).name }
      results
    end,
    'findPickleStepBy' => ->(query) { query.find_all_test_steps.filter_map { |message| query.find_pickle_step_by(message)&.text } },
    'findStepBy' => ->(query) { query.find_all_pickle_steps.map { |message| query.find_step_by(message).text } },
    'findStepDefinitionsBy' => ->(query) { query.find_all_test_steps.map { |message| query.find_step_definitions_by(message).map(&:id) } },
    'findTestCaseBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_test_case_by(message).id }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_test_case_by(message).id }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_test_case_by(message).id }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_test_case_by(message).id }
      results
    end,
    'findTestCaseStartedBy' => lambda do |query|
      results = {}
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_test_case_started_by(message).id }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_test_case_started_by(message).id }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_test_case_started_by(message).id }
      results
    end,
    'findTestCaseFinishedBy' => ->(query) { query.find_all_test_case_started.map { |message| query.find_test_case_finished_by(message).test_case_started_id } },
    'findTestRunDuration' => lambda(&:find_test_run_duration),
    'findTestRunHookStartedBy' => ->(query) { query.find_all_test_run_hook_started.map(&:id) },
    'findTestRunHookFinishedBy' => ->(query) { query.find_all_test_run_hook_finished.map(&:test_run_hook_started_id) },
    'findTestRunStarted' => lambda do |query|
      results = {}
      message = query.find_test_run_started
      results['id'] = message.id
      results['timestamp'] = { 'nanos' => message.timestamp.nanos, 'seconds' => message.timestamp.seconds }
      results
    end,
    'findTestRunFinished' => lambda do |query|
      results = {}
      message = query.find_test_run_finished
      results['success'] = message.success
      results['testRunStartedId'] = message.test_run_started_id
      results['timestamp'] = { 'nanos' => message.timestamp.nanos, 'seconds' => message.timestamp.seconds }
      results
    end,
    'findTestStepBy' => lambda do |query|
      results = {}
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_test_step_by(message).id }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_test_step_by(message).id }
      results
    end
  }

  sources = %w[
    attachments
    empty
    examples-tables
    global-hooks
    global-hooks-attachments
    hooks
    minimal
    rules
    unknown-parameter-type
  ]

  subject(:query) { described_class.new(repository) }

  let(:repository) { Cucumber::Repository.new }

  sources.product(queries.to_a).each do |source_name, (query_name, query_proc)|
    describe "Query: '#{query_name}'. testdata source: '#{source_name}'" do
      let(:source) { testdata_path("#{source_name}.ndjson") }
      let(:expected_results_file) { testdata_path("#{source_name}.#{query_name}.results.json") }
      let(:messages) { parse_ndjson_file(source) }

      before do
        messages.each { |message| repository.update(message) }
      end

      it 'returns the expected query result' do
        evaluated_query = query_proc.call(query)
        expected_query_result = JSON.parse(File.read(expected_results_file))

        expect(evaluated_query).to eq(expected_query_result)
      end
    end
  end
end
