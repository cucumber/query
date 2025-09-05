# frozen_string_literal: true

require 'cucumber/query/pickle_step_by_test_step'

describe Cucumber::Query::PickleStepByTestStep do
  before do
    @test_cases = []
    @config = actual_runtime.configuration.with_options(out_stream: StringIO.new)
    @formatter = described_class.new(@config)

    @config.on_event(:test_case_created) do |event|
      @test_cases << event.test_case
    end

    @pickle_step_ids = []
    @config.on_event(:envelope) do |event|
      next unless event.envelope.pickle

      event.envelope.pickle.steps.each do |step|
        @pickle_step_ids << step.id
      end
    end
  end

  let(:first_test_case) { @test_cases.first }

  describe 'given a single feature' do
    before do
      run_defined_feature
    end

    describe 'with a scenario' do
      describe '#pickle_step_id' do
        define_feature <<-FEATURE
          Feature: Banana party

            Scenario: Monkey eats banana
              Given there are bananas
        FEATURE

        it 'provides the ID of the PickleStep used to generate the Test::Step' do
          test_step = first_test_case.test_steps.first

          expect(@formatter.pickle_step_id(test_step)).to eq(@pickle_step_ids.first)
        end

        it 'raises an exception when the test_step is unknown' do
          test_step = double
          allow(test_step).to receive(:id).and_return('whatever-id')

          expect { @formatter.pickle_step_id(test_step) }.to raise_error(Cucumber::Query::TestStepUnknownError)
        end
      end
    end
  end
end
