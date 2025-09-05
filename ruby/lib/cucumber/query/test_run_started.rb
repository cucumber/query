# frozen_string_literal: true

require_relative 'errors'

module Cucumber
  module Query
    class TestRunStarted
      def initialize(config)
        @config = config
      end

      def id
        @id ||= @config.id_generator.new_id
      end
    end
  end
end
