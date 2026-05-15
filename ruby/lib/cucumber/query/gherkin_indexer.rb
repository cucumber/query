# frozen_string_literal: true

module Cucumber
  module Query
    class GherkinIndexer
      def initialize(state)
        @state = state
      end

      def update(gherkin_document)
        return unless gherkin_document.feature

        update_feature(gherkin_document.feature, { gherkin_document: gherkin_document })
      end

      private

      def update_feature(feature, lineage)
        feature.children.each do |feature_child|
          lineage = update_background(feature_child.background, lineage, :background) if feature_child.background
          update_scenario(feature_child.scenario, lineage.merge(feature: feature)) if feature_child.scenario
          update_rule(feature_child.rule, lineage.merge(feature: feature)) if feature_child.rule
        end
      end

      def update_rule(rule, lineage)
        rule.children.each do |rule_child|
          lineage = update_background(rule_child.background, lineage, :rule_background) if rule_child.background
          update_scenario(rule_child.scenario, lineage.merge(rule: rule)) if rule_child.scenario
        end
      end

      def update_background(background, lineage, key)
        update_steps(background.steps)
        lineage.merge(key => background)
      end

      def update_scenario(scenario, lineage)
        @state.lineage_by_id[scenario.id] = lineage.merge(scenario: scenario)
        scenario.examples.each_with_index do |examples, examples_index|
          update_examples_lineage(scenario, lineage, examples, examples_index)
        end
        update_steps(scenario.steps)
      end

      def update_examples_lineage(scenario, lineage, examples, examples_index)
        examples_lineage = lineage.merge(scenario: scenario, examples: examples, examples_index: examples_index)
        @state.lineage_by_id[examples.id] = examples_lineage
        examples.table_body.each_with_index do |example, example_index|
          @state.lineage_by_id[example.id] = examples_lineage.merge(example: example, example_index: example_index)
        end
      end

      def update_steps(steps)
        steps.each { |step| @state.step_by_id[step.id] = step }
      end
    end
  end
end
