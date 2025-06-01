#!/usr/bin/env python3
# Simple Python test script

import os
import sys
import datetime

# Print HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Print HTML
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Python Test</title>
    <link rel="stylesheet" href="/styles.css">
</head>
<body>
    <div class="container">
        <h1>Python Test Script</h1>
        <div class="card">
            <h2>Basic Test</h2>
            <p>Current time: {}</p>
            <p>Python version: {}</p>
            <p>This is a simple test script to verify Python CGI is working.</p>
        </div>
        <p><a href="/">Back to Home</a></p>
    </div>
</body>
</html>""".format(
    datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    sys.version.split()[0]
))
