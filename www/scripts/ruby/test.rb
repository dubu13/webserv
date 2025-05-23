#!/usr/bin/ruby

puts "Content-type: text/html\n\n"
puts "<html>"
puts "<head>"
puts "<title>Ruby CGI Test</title>"
puts "</head>"
puts "<body>"
puts "<h1>Ruby CGI Test</h1>"
puts "<p>This is a test script for Ruby CGI support.</p>"
puts "<h2>Environment Variables:</h2>"
puts "<ul>"
ENV.sort.each do |key, value|
  puts "<li>#{key}: #{value}</li>"
end
puts "</ul>"
puts "</body>"
puts "</html>"