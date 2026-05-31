# frozen_string_literal: true

require 'json'
require 'cucumber/query'

RSpec.describe Cucumber::Query do
  SOURCE_NAMES = %w[attachments empty hooks minimal rules].freeze

  QUERIES = {
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
    'findPickleBy' => lambda do |query|
      results = {}
      results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_pickle_by(message).name }
      results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_pickle_by(message).name }
      results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_pickle_by(message).name }
      results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_pickle_by(message).name }
      results
    end
  }.freeze

  def testdata_path(name)
    File.expand_path("../support/#{name}", __dir__)
  end

  def parse_ndjson_file(path)
    Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(File.read(path))
  end

  subject(:query) { described_class.new(repository) }

  let(:repository) { Cucumber::Repository.new }

  SOURCE_NAMES.product(QUERIES.to_a).each do |source_name, (query_name, query_proc)|
    describe "executes the query '#{query_name}' against the testdata source '#{source_name}'" do
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
