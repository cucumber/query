# Cucumber Query for Ruby

`cucumber-query` builds an in-memory index of Cucumber Messages so formatters
and other message consumers can answer questions such as "which pickle belongs
to this test case?", "which step definition matched this test step?", or "what
was the most severe result for this scenario?".

The Ruby implementation follows the public API shape used by the other
`cucumber-query` packages while using Ruby snake_case method names.

## Installation

```ruby
gem 'cucumber-query', '~> 15.0'
```

Ruby 3.2 or newer is required. The gem depends on `cucumber-messages` and
accepts `Cucumber::Messages::Envelope` objects.

## Basic usage

Create one query per message stream, then pass every envelope to `#update` in
the order it was produced:

```ruby
require 'cucumber/query'
require 'cucumber/messages/helpers/ndjson_to_message_enumerator'

query = Cucumber::Query::Query.new

File.open('cucumber-messages.ndjson', 'r') do |io|
  Cucumber::Messages::Helpers::NdjsonToMessageEnumerator.new(io).each do |envelope|
    query.update(envelope)
  end
end

puts query.count_test_cases_started
puts query.find_all_pickles.map(&:name)
```

`#update` returns `nil`. After each update, the query indexes the parts of the
envelope it understands and keeps the original envelope in `#envelopes` for
inspection/debugging.

## Message flow and architecture

The query is deliberately stateful:

1. `meta`, `gherkin_document`, `pickle`, `hook`, `step_definition`, `test_case`
   and other definition messages populate lookup tables.
2. Runtime messages such as `test_case_started`, `test_step_finished`,
   `attachment`, `test_case_finished`, and global hook messages are linked back
   to those definitions by id.
3. Query methods read those lookup tables and return message objects from
   `cucumber-messages`, not wrapper objects.

Because Cucumber Messages refer to related data by id, feed the complete stream
to the query when possible. If a formatter asks for a pickle before the
corresponding `pickle` message has been seen, the lookup will return `nil`.

## Common query methods

Counts and result aggregation:

```ruby
query.count_test_cases_started
query.count_most_severe_test_step_result_status
```

Top-level collections:

```ruby
query.find_all_pickles
query.find_all_pickle_steps
query.find_all_step_definitions
query.find_all_test_steps
query.find_all_test_case_started
query.find_all_test_case_finished
query.find_all_test_run_hook_started
query.find_all_test_run_hook_finished
query.find_all_undefined_parameter_types
```

Relationship lookups:

```ruby
query.find_pickle_by(test_case_started_or_finished)
query.find_location_of(pickle)
query.find_lineage_by(pickle_or_runtime_message)
query.find_test_case_by(test_case_started_or_finished)
query.find_test_case_started_by(test_case_finished_or_step_message)
query.find_test_case_finished_by(test_case_started)
query.find_test_step_by(test_step_started_or_finished)
query.find_pickle_step_by(test_step)
query.find_step_by(pickle_step)
query.find_hook_by(test_step_or_test_run_hook_message)
query.find_step_definitions_by(test_step)
query.find_unambiguous_step_definition_by(test_step)
query.find_attachments_by(test_step_finished_or_test_run_hook_finished)
query.find_suggestions_by(pickle_or_pickle_step)
```

Duration helpers:

```ruby
query.find_test_case_duration_by(test_case_started_or_finished)
query.find_test_run_duration
```

Ordering helpers:

```ruby
query.find_all_test_case_started_order_by(->(query, item) { query.find_pickle_by(item).name }, ->(a, b) { a <=> b })
query.find_all_test_case_finished_order_by(->(query, item) { item.timestamp.seconds }, ->(a, b) { a <=> b })
```

The ordering methods receive a projection callable and a comparison callable.
Items whose projection is `nil` sort after items with a value.

## Local development dependencies

Use released gems by default. To test against a local `cucumber-messages`
checkout without editing this repository, set an environment variable before
running Bundler:

```sh
cd repos/query/ruby
CUCUMBER_MESSAGES_RUBY_PATH=../../messages/ruby bundle install
```

Run the Ruby checks from this directory:

```sh
bundle exec rake
```

The acceptance specs load shared NDJSON fixtures from `../testdata/src` and
compare the Ruby query results with the shared expected JSON files.

## Performance and thread-safety notes

`Cucumber::Query::Query` keeps indexes for the whole message stream in memory.
This is the intended trade-off for formatters that need to correlate runtime
messages with source, pickle, hook, attachment, and suggestion messages.

A query instance is mutable and is not designed for concurrent `#update` calls.
Use one query per formatter/message stream, or synchronize access externally if
you share an instance across threads.

## Troubleshooting

- A lookup returning `nil` usually means the related message has not been fed to
  the query yet, or the input stream is incomplete.
- Retried scenarios whose `test_case_finished.will_be_retried` flag is true are
  excluded from final scenario/result collections such as
  `find_all_test_case_started` and `find_all_test_case_finished`.
- Status aggregation uses Cucumber's severity order:
  `UNKNOWN`, `PASSED`, `SKIPPED`, `PENDING`, `UNDEFINED`, `AMBIGUOUS`, `FAILED`.
- The Ruby package is developed against shared Cucumber Query testdata; when a
  new shared fixture appears, add/adjust Ruby acceptance coverage rather than
  creating Ruby-only behaviour.
