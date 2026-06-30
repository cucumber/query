# frozen_string_literal: true

require 'json'
require 'cucumber/query'

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
    'findAllTestSteps' => ->(query) { query.find_all_test_steps.length },
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
    end,
    'findPickleBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_pickle_by(message).name }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_pickle_by(message).name }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_pickle_by(message).name }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_pickle_by(message).name }
      results
    end
  }.freeze

  # This list needs to be tightly controlled.
  #   New items should be added when new queries have been adapted to work with them
  #     AND
  #   When the requisite sample data result files have been created
  enabled_files = %w[attachments empty examples-tables global-hooks hooks minimal rules unknown-parameter-type].freeze

  files_in_testdata = Dir.glob(File.expand_path("../../../testdata/src/*.ndjson", __dir__))
  files_in_testdata = files_in_testdata.select do |file|
    enabled_files.any? do |enabled_file|
      file.match?("/#{enabled_file}.ndjson")
    end
  end
  filenames_in_testdata = files_in_testdata.map { it.split('/').last.split('.').first }

  subject(:query) { described_class.new(repository) }

  let(:repository) { Cucumber::Repository.new }

  filenames_in_testdata.product(queries.to_a).each do |source_name, (query_name, query_proc)|
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
