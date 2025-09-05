# frozen_string_literal: true

require 'cucumber/query/hook_by_test_step'

describe Cucumber::Query::HookByTestStep do
  before do
    @test_cases = []
    @hook_ids = []

    config.on_event(:test_case_started) do |event|
      @test_cases << event.test_case
    end

    config.on_event(:envelope) do |event|
      next unless event.envelope.hook

      @hook_ids << event.envelope.hook.id
    end
  end

  let(:config) { actual_runtime.configuration.with_options(out_stream: StringIO.new) }
  let(:first_test_case) { @test_cases.first }
  let(:formatter) { described_class.new(config) }

  context 'given a single feature' do
    before do
      run_defined_feature
    end

    context 'with a scenario' do
      describe '#pickle_step_id' do
        define_feature <<-FEATURE
          Feature: Banana party

            Scenario: Monkey eats banana
              Given there are bananas
        FEATURE

        define_steps do
          Before() {}
          After() {}
        end

        it 'provides the ID of the Before Hook used to generate the Test::Step' do
          expect(formatter.hook_id(first_test_case.test_steps.first)).to eq(@hook_ids.first)
        end

        it 'provides the ID of the After Hook used to generate the Test::Step' do
          expect(formatter.hook_id(first_test_case.test_steps.last)).to eq(@hook_ids.last)
        end

        it 'returns nil if the step was not generated from a hook' do
          expect(formatter.hook_id(first_test_case.test_steps[1])).to be_nil
        end

        it 'raises an exception when the test_step is unknown' do
          test_step = double
          allow(test_step).to receive(:id).and_return('whatever-id')

          expect { formatter.hook_id(test_step) }.to raise_error(Cucumber::Query::TestStepUnknownError)
        end
      end
    end

    context 'with AfterStep hooks' do
      describe '#pickle_step_id' do
        define_feature <<-FEATURE
          Feature: Banana party

            Scenario: Monkey eats banana
              Given there are bananas
        FEATURE

        define_steps do
          AfterStep() {}
        end

        it 'provides the ID of the AfterStepHook used to generate the Test::Step' do
          expect(formatter.hook_id(first_test_case.test_steps.last)).to eq(@hook_ids.first)
        end
      end
    end
  end
end
