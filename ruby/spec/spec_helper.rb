# frozen_string_literal: true

require 'cucumber'
require 'cucumber/core'

require_relative 'support/runner_helper'
require_relative 'support/gherkin_helper'

RSpec.configure do |c|
  c.include RunnerHelper
  c.extend GherkinHelper
end
