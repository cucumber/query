# frozen_string_literal: true

module Cucumber
  module Query
    class ResultOrder
      def initialize(find_order_by, order)
        @find_order_by = find_order_by
        @order = order
      end

      def sort(query, items)
        items.map { |item| [item, @find_order_by.call(query, item)] }
             .sort { |left, right| compare(left.last, right.last) }
             .map(&:first)
      end

      private

      def compare(left, right)
        return 0 if left.nil? && right.nil?
        return 1 if left.nil?
        return -1 if right.nil?

        @order.call(left, right)
      end
    end
  end
end
