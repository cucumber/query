# frozen_string_literal: true

require 'cucumber/query/pickle_by_test'

describe Cucumber::Query::PickleByTest do
  before do
    @test_cases = []
    @pickle_ids = []

    config.on_event(:test_case_created) do |event|
      @test_cases << event.test_case
    end

    config.on_event(:envelope) do |event|
      next unless event.envelope.pickle

      @pickle_ids << event.envelope.pickle.id
    end
  end

  let(:config) { actual_runtime.configuration.with_options(out_stream: StringIO.new) }
  let(:formatter) { described_class.new(config) }

  describe 'given a single feature' do
    before do
      run_defined_feature
    end

    describe 'with a scenario' do
      describe '#pickle_id' do
        define_feature <<-FEATURE
          Feature: Banana party

            Scenario: Monkey eats banana
              Given there are bananas
        FEATURE

        it 'provides the ID of the pickle used to generate the Test::Case' do
          expect(formatter.pickle_id(@test_cases.first)).to eq(@pickle_ids.first)
        end

        it 'raises an error when the Test::Case is unknown' do
          test_case = double
          allow(test_case).to receive(:id).and_return('whatever-id')

          expect { formatter.pickle_id(test_case) }.to raise_error(Cucumber::Query::TestCaseUnknownError)
        end
      end
    end
  end
end
