# frozen_string_literal: true

module Cucumber
  module Query
    module Error
      TestCaseUnknownError = Class.new(StandardError)
      TestStepUnknownError = Class.new(StandardError)
    end
  end
end
