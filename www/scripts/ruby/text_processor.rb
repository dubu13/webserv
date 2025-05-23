#!/usr/bin/ruby
require 'json'

# Sample text for processing demonstration
sample_text = <<-TEXT
The WebServ project implements a HTTP/1.1 compliant web server in C++.
It handles GET, POST and DELETE methods, can serve static files,
process CGI scripts, and provides various configuration options.
This demonstrates Ruby's text processing capabilities through CGI.
TEXT

# Apply text processing operations
word_count = sample_text.split.size
char_count = sample_text.length
line_count = sample_text.lines.count
most_common_words = sample_text.downcase.gsub(/[^\w\s]/, '').split
  .group_by(&:itself)
  .transform_values(&:count)
  .sort_by { |_, count| -count }
  .first(5)
  .map { |word, count| { word: word, count: count } }

# Create syntax highlighted version
highlighted_text = sample_text.gsub(/(GET|POST|DELETE)/, '<span class="highlight-keyword">\1</span>')
highlighted_text = highlighted_text.gsub(/(HTTP\/1\.1|C\+\+|CGI)/, '<span class="highlight-function">\1</span>')
highlighted_text = highlighted_text.gsub(/(WebServ)/, '<span class="highlight-string">\1</span>')

# Create reversed text
reversed_text = sample_text.split(/\s+/).map(&:reverse).join(' ')

# Generate word length statistics
word_lengths = sample_text.split.map(&:length)
max_length = word_lengths.max
word_length_counts = Hash.new(0)
word_lengths.each { |length| word_length_counts[length] += 1 }

# Get system information
hostname = `hostname`.strip
ruby_version = RUBY_VERSION
platform = RUBY_PLATFORM
current_time = Time.now.strftime("%Y-%m-%d %H:%M:%S")
kernel = `uname -r`.strip

# Create a Hash of demo text processing functions
demos = [
  {
    name: "Regex Matching",
    code: "sample_text.scan(/[A-Z][a-z]+/).join(', ')",
    result: sample_text.scan(/[A-Z][a-z]+/).join(', ')
  },
  {
    name: "Map Transform",
    code: "sample_text.split.map(&:capitalize).join(' ')",
    result: sample_text.split.map(&:capitalize).join(' ')
  },
  {
    name: "Functional Filter",
    code: "sample_text.split.select { |word| word.length > 5 }.join(' ')",
    result: sample_text.split.select { |word| word.length > 5 }.join(' ')
  },
  {
    name: "String Interpolation",
    code: '"There are #{word_count} words in the text"',
    result: "There are #{word_count} words in the text"
  },
  {
    name: "One-line Sort",
    code: "sample_text.split.sort_by(&:length).join(' ')",
    result: sample_text.split.sort_by(&:length).join(' ')
  }
]

# Create JSON output
output = {
  system_info: {
    ruby_version: ruby_version,
    platform: platform,
    hostname: hostname,
    current_time: current_time,
    kernel: kernel
  },
  text_analysis: {
    sample_text: sample_text,
    word_count: word_count,
    char_count: char_count,
    line_count: line_count,
    most_common_words: most_common_words,
    word_length_stats: word_length_counts.map { |length, count| { length: length, count: count } }
  },
  transformations: {
    highlighted_text: highlighted_text,
    reversed_text: reversed_text
  },
  demos: demos
}

# Output the JSON with content type header
puts "Content-type: application/json\n\n"
puts JSON.pretty_generate(output)