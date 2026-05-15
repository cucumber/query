# frozen_string_literal: true

module Cucumber
  module Query
    NAMING_STRATEGY_LENGTH_LONG = :long
    NAMING_STRATEGY_LENGTH_SHORT = :short

    NAMING_STRATEGY_FEATURE_NAME_INCLUDE = :include
    NAMING_STRATEGY_FEATURE_NAME_EXCLUDE = :exclude

    NAMING_STRATEGY_EXAMPLE_NAME_NUMBER = :number
    NAMING_STRATEGY_EXAMPLE_NAME_PICKLE = :pickle
    NAMING_STRATEGY_EXAMPLE_NAME_NUMBER_AND_PICKLE_IF_PARAMETERIZED = :number_and_pickle_if_parameterized

    # Reduces a Gherkin lineage and pickle into the test name used by polyglot formatters.
    class BuiltinNamingStrategy
      def initialize(length:, feature_name: NAMING_STRATEGY_FEATURE_NAME_INCLUDE,
                     example_name: NAMING_STRATEGY_EXAMPLE_NAME_NUMBER_AND_PICKLE_IF_PARAMETERIZED)
        @length = length
        @feature_name = feature_name
        @example_name = example_name
      end

      def reduce(lineage, pickle)
        filtered_parts = parts(lineage, pickle).compact.reject(&:empty?)
        return filtered_parts.last if @length == NAMING_STRATEGY_LENGTH_SHORT

        filtered_parts.join(' - ')
      end

      private

      def parts(lineage, pickle)
        [
          feature_name(lineage),
          lineage[:rule]&.name,
          lineage[:scenario]&.name || pickle.name,
          lineage[:examples]&.name,
          lineage[:example] && example_name(lineage, pickle)
        ]
      end

      def feature_name(lineage)
        lineage[:feature]&.name if @feature_name == NAMING_STRATEGY_FEATURE_NAME_INCLUDE
      end

      def example_name(lineage, pickle)
        example_number = example_number(lineage)
        return example_number if @example_name == NAMING_STRATEGY_EXAMPLE_NAME_NUMBER
        return pickle.name if @example_name == NAMING_STRATEGY_EXAMPLE_NAME_PICKLE
        return parameterized_example_name(lineage, pickle, example_number) if parameterized_example_strategy?

        raise ArgumentError, "unknown example naming strategy: #{@example_name.inspect}"
      end

      def example_number(lineage) = "##{lineage.fetch(:examples_index, 0) + 1}.#{lineage.fetch(:example_index, 0) + 1}"

      def parameterized_example_name(lineage, pickle, example_number)
        lineage[:scenario]&.name == pickle.name ? example_number : "#{example_number}: #{pickle.name}"
      end

      def parameterized_example_strategy?
        @example_name == NAMING_STRATEGY_EXAMPLE_NAME_NUMBER_AND_PICKLE_IF_PARAMETERIZED
      end
    end

    module_function

    def naming_strategy(length, feature_name = NAMING_STRATEGY_FEATURE_NAME_INCLUDE,
                        example_name = NAMING_STRATEGY_EXAMPLE_NAME_NUMBER_AND_PICKLE_IF_PARAMETERIZED)
      BuiltinNamingStrategy.new(length:, feature_name:, example_name:)
    end
  end
end
