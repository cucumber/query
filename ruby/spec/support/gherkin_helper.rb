# TODO: This file is adapted / duplicated from SpecHelperDSL and SpecHelper in cucumber-ruby

module GherkinHelper
  attr_reader :feature_content, :step_definitions, :feature_filename

  def define_feature(string, feature_file = 'spec.feature')
    @feature_content = string
    @feature_filename = feature_file
  end

  def define_steps(&block)
    @step_definitions = block
  end
end
