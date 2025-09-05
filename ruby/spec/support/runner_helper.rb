# TODO: This file is adapted / duplicated from SpecHelperDSL and SpecHelper in cucumber-ruby

module RunnerHelper

  include Cucumber::Core

  def run_defined_feature
    define_steps
    actual_runtime.visitor = Cucumber::Formatter::Fanout.new([@formatter])
    receiver = Test::Runner.new(event_bus)

    event_bus.gherkin_source_read(gherkin_doc.uri, gherkin_doc.body)

    compile [gherkin_doc], receiver, filters, event_bus

    event_bus.test_run_finished
  end

  def filters
    [
      Cucumber::Filters::ActivateSteps.new(
        Cucumber::StepMatchSearch.new(actual_runtime.support_code.registry.method(:step_matches), actual_runtime.configuration),
        actual_runtime.configuration
      ),
      Cucumber::Filters::ApplyAfterStepHooks.new(actual_runtime.support_code),
      Cucumber::Filters::ApplyBeforeHooks.new(actual_runtime.support_code),
      Cucumber::Filters::ApplyAfterHooks.new(actual_runtime.support_code),
      Cucumber::Filters::ApplyAroundHooks.new(actual_runtime.support_code),
      Cucumber::Filters::BroadcastTestRunStartedEvent.new(actual_runtime.configuration),
      Cucumber::Filters::BroadcastTestCaseReadyEvent.new(actual_runtime.configuration),
      Cucumber::Filters::PrepareWorld.new(actual_runtime)
    ]
  end

  def gherkin_doc
    Cucumber::Core::Gherkin::Document.new(self.class.feature_filename, gherkin)
  end

  def gherkin
    self.class.feature_content || raise('No feature content defined!')
  end

  def actual_runtime
    @actual_runtime ||= Cucumber::Runtime.new({})
  end

  def event_bus
    actual_runtime.configuration.event_bus
  end

  def define_steps
    step_definitions = self.class.step_definitions

    return unless step_definitions

    dsl = Object.new
    dsl.extend Cucumber::Glue::Dsl
    dsl.instance_exec(&step_definitions)
  end
end
