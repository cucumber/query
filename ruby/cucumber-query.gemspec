# frozen_string_literal: true

Gem::Specification.new do |s|
  s.name        = 'cucumber-query'
  s.version     = File.read(File.expand_path('VERSION', __dir__)).strip
  s.authors     = ['Luke Hill']
  s.description = 'Cucumber Query - query messages'
  s.summary     = "#{s.name}-#{s.version}"
  s.email       = 'cukebot@cucumber.io'
  s.homepage    = 'https://github.com/cucumber/query'
  s.platform    = Gem::Platform::RUBY
  s.license     = 'MIT'
  s.required_ruby_version = '>= 3.2'
  s.required_rubygems_version = '>= 3.2.8'

  s.metadata = {
    'bug_tracker_uri' => 'https://github.com/cucumber/query/issues',
    'changelog_uri' => 'https://github.com/cucumber/query/blob/main/CHANGELOG.md',
    'documentation_uri' => 'https://github.com/cucumber/query/blob/main/CONTRIBUTING.md',
    'mailing_list_uri' => 'https://groups.google.com/forum/#!forum/cukes',
    'source_code_uri' => 'https://github.com/cucumber/query/blob/main/ruby'
  }

  s.add_development_dependency 'rspec', '~> 3.13'
  s.add_development_dependency 'rubocop', '~> 1.80.0'
  s.add_development_dependency 'rubocop-performance', '~> 1.25.0'
  s.add_development_dependency 'rubocop-rspec', '~> 3.7.0'

  s.files            = Dir['README.md', 'LICENSE', 'lib/**/*']
  s.rdoc_options     = ['--charset=UTF-8']
  s.require_path     = 'lib'
end
