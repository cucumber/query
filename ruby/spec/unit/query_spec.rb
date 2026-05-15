# frozen_string_literal: true

RSpec.describe Cucumber::Query::Query do
  it 'accepts message envelopes via #update' do
    envelope = Cucumber::Messages::Envelope.new
    query = described_class.new

    expect(query.update(envelope)).to be_nil
    expect(query.envelopes).to eq([envelope])
  end

  describe '#find_lineage_by' do
    it 'returns nil when the related pickle cannot be found' do
      test_case_started = Struct.new(:test_case_id).new('missing-test-case')
      query = described_class.new

      expect(query.find_lineage_by(test_case_started)).to be_nil
    end

    it 'returns nil when the pickle has no AST node id' do
      pickle = Struct.new(:ast_node_ids).new([])
      query = described_class.new

      expect(query.find_lineage_by(pickle)).to be_nil
    end
  end
end
