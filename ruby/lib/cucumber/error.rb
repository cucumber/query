# frozen_string_literal: true

module Cucumber
  module Error
    TestCaseUnknownError = Class.new(StandardError)
    TestStepUnknownError = Class.new(StandardError)
  end
end
