# frozen_string_literal: true

RSpec.describe Cucumber::Query do
  def testdata_path(name)
    File.expand_path("../../../testdata/src/#{name}", __dir__)
  end

  def parse_ndjson_file(path)
    Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(File.read(path))
  end

  queries = {
    'findAllPickles' => ->(query) { query.find_all_pickles.length },
    'findAllTestCases' => ->(query) { query.find_all_test_cases.length },
    'findAllTestRunHookStarted' => ->(query) { query.find_all_test_run_hook_started },
    'findAllTestRunHookFinished' => ->(query) { query.find_all_test_run_hook_finished },
    'findAllTestSteps' => ->(query) { query.find_all_test_steps.length },
    'findMeta' => ->(query) { query.find_meta },
    'findPickleBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_pickle_by(message).name }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_pickle_by(message).name }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_pickle_by(message).name }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_pickle_by(message).name }
      results
    end,
    'findTestCaseBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_test_case_by(message).id }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_test_case_by(message).id }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_test_case_by(message).id }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_test_case_by(message).id }
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
