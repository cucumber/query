# frozen_string_literal: true

require 'json'
require 'cucumber/query'

source_names = %w[attachments empty hooks minimal rules].freeze

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
  'findPickleBy' => lambda do |query|
    results = {}
    results['testCaseStarted'] = query.find_all_test_case_started.map { |message| query.find_pickle_by(message).name }
    results['testCaseFinished'] = query.find_all_test_case_finished.map { |message| query.find_pickle_by(message).name }
    results['testStepStarted'] = query.find_all_test_step_started.map { |message| query.find_pickle_by(message).name }
    results['testStepFinished'] = query.find_all_test_step_finished.map { |message| query.find_pickle_by(message).name }
    results
  end
}.freeze

RSpec.describe Cucumber::Query do
  def testdata_path(name)
    File.expand_path("../../../testdata/src/#{name}", __dir__)
  end

  def parse_ndjson_file(path)
    Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(File.read(path))
  end

  subject(:query) { described_class.new(repository) }

  let(:repository) { Cucumber::Repository.new }

  describe 'packaged files' do
    it 'can require every packaged library file after requiring cucumber/query' do
      gemspec = Gem::Specification.load(File.expand_path('../../cucumber-query.gemspec', __dir__))
      packaged_files = gemspec.files.grep(%r{\Alib/.*\.rb\z})
      packaged_requires = packaged_files.map { |file| file.delete_prefix('lib/').delete_suffix('.rb') }

      packaged_requires.each do |path|
        expect { require path }.not_to raise_error
      end
    end
  end

  describe '#find_test_step_by' do
    let(:source) { testdata_path('minimal.ndjson') }
    let(:messages) { parse_ndjson_file(source) }

    before do
      messages.each { |message| repository.update(message) }
    end

    it 'finds test steps by TestStepStarted messages' do
      expect(query.find_all_test_step_started).not_to be_empty

      query.find_all_test_step_started.each do |test_step_started|
        expect(query.find_test_step_by(test_step_started).id).to eq(test_step_started.test_step_id)
      end
    end

    it 'finds test steps by TestStepFinished messages' do
      expect(query.find_all_test_step_finished).not_to be_empty

      query.find_all_test_step_finished.each do |test_step_finished|
        expect(query.find_test_step_by(test_step_finished).id).to eq(test_step_finished.test_step_id)
      end
    end
  end

  source_names.product(queries.to_a).each do |source_name, (query_name, query_proc)|
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
