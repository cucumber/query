
module GherkinHelper
  attr_reader :feature_content, :step_defs, :feature_filename

  def define_feature(string, feature_file = 'spec.feature')
    @feature_content = string
    @feature_filename = feature_file
  end

  def define_steps(&block)
    @step_defs = block
  end
end
