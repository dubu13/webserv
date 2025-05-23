#!/bin/bash

echo "Content-type: text/html"
echo ""

echo "<html>"
echo "<head>"
echo "<title>Bash CGI Test</title>"
echo "</head>"
echo "<body>"
echo "<h1>Bash CGI Test</h1>"
echo "<p>This is a test script for Bash CGI support.</p>"
echo "<h2>Environment Variables:</h2>"
echo "<ul>"
env | sort | while read line; do
  echo "<li>$line</li>"
done
echo "</ul>"
echo "</body>"
echo "</html>"