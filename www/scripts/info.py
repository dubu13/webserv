#!/usr/bin/env python3
# Simple Python CGI script for WebServ testing

import os
import sys
import datetime

# Print HTTP headers
print("Content-Type: text/html")
print()  # Empty line to separate headers from body

# Start HTML output
print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Python CGI Test</title>
    <link rel="stylesheet" href="/styles.css">
</head>
<body>
    <div class="container">
        <h1>Python CGI is working!</h1>
        <div class="card">
            <h2>Environment Information</h2>
            <ul>""")

# Print some environment variables
for key in sorted(['SERVER_SOFTWARE', 'SERVER_NAME', 'GATEWAY_INTERFACE', 
                   'SERVER_PROTOCOL', 'SERVER_PORT', 'REQUEST_METHOD', 
                   'PATH_INFO', 'QUERY_STRING', 'REMOTE_ADDR']):
    if key in os.environ:
        print(f"<li><strong>{key}:</strong> {os.environ.get(key, 'Not set')}</li>")

print("""</ul>
        </div>
        
        <div class="card">
            <h2>System Information</h2>
            <ul>
                <li><strong>Python Version:</strong> {}</li>
                <li><strong>Date/Time:</strong> {}</li>
                <li><strong>Script Path:</strong> {}</li>
            </ul>
        </div>""".format(
            sys.version.split()[0],
            datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            os.environ.get('SCRIPT_FILENAME', 'Unknown')
        ))

# Handle form data if this is a POST request
if os.environ.get('REQUEST_METHOD') == 'POST':
    print("""
        <div class="card">
            <h2>POST Data Received</h2>
            <p>Your form data was successfully received by the CGI script.</p>
        </div>""")

print("""
        <div class="card">
            <h2>Test Form (POST method)</h2>
            <form action="/scripts/info.py" method="post">
                <div class="form-group">
                    <label for="name">Your name:</label>
                    <input type="text" name="name" id="name">
                </div>
                <div class="form-group">
                    <button type="submit">Submit</button>
                </div>
            </form>
        </div>
        
        <p><a href="/">Back to Home</a></p>
    </div>
</body>
</html>""")
