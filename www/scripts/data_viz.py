#!/usr/bin/env python3

import os
import sys

def main():
    # Set content type
    print("Content-Type: text/html; charset=utf-8")
    print()  # Empty line required after headers
    
    # Get the request method
    method = os.environ.get('REQUEST_METHOD', 'GET')
    
    print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Data Visualization Demo</title>
    <link rel="stylesheet" href="/styles.css">
    <style>
        .chart-container { margin: 20px 0; padding: 20px; border: 2px solid #3498db; border-radius: 5px; background-color: #ecf0f1; }
        .bar { display: inline-block; width: 30px; margin: 2px; background-color: #3498db; color: white; text-align: center; font-weight: bold; }
        .result { margin: 20px 0; padding: 15px; background-color: #d4edda; border: 1px solid #c3e6cb; border-radius: 5px; }
        .error { background-color: #f8d7da; border-color: #f5c6cb; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; align-items: center; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Data Visualization Demo</h1>
        <div class="card">
            <p>This demonstrates Python CGI handling both GET and POST requests with data processing.</p>""")
    
    if method == 'POST':
        # Handle POST data
        form = cgi.FieldStorage()
        
        # Get data points from form
        data_points = []
        labels = []
        
        try:
            # Extract multiple data points
            for i in range(1, 4):  # Support up to 3 data points for simplicity
                label_key = f'label{i}'
                value_key = f'value{i}'
                
                if label_key in form and value_key in form:
                    label = form[label_key].value.strip()
                    value = form[value_key].value.strip()
                    
                    if label and value:
                        labels.append(label)
                        data_points.append(int(value))
            
            if data_points:
                print('<div class="result">')
                print('<h3>Your Data Visualization</h3>')
                print('<div class="chart-container">')
                
                # Create a simple bar chart
                max_value = max(data_points) if data_points else 1
                for i, (label, value) in enumerate(zip(labels, data_points)):
                    height = max(20, int(value * 100 / max_value))  # Scale to max 100px
                    print(f'<div style="display: inline-block; margin: 10px; text-align: center;">')
                    print(f'  <div class="bar" style="height: {height}px; vertical-align: bottom;">{value}</div>')
                    print(f'  <div style="margin-top: 5px; font-weight: bold;">{label}</div>')
                    print(f'</div>')
                
                print('</div>')
                print(f'<p>Average: {sum(data_points) / len(data_points):.1f}</p>')
                print('</div>')
            else:
                print('<div class="result error">')
                print('<p>No valid data points provided.</p>')
                print('</div>')
                
        except Exception as e:
            print('<div class="result error">')
            print(f'<p>Error: {str(e)}</p>')
            print('</div>')
    
    # Always show the form
    print("""
        </div>
        <div class="card">
            <h2>Enter Your Data</h2>
            <form method="post" action="/scripts/data_viz.py">
                <div class="grid">
                    <label>Label 1:</label>
                    <input type="text" name="label1" placeholder="Sales" />
                    <label>Value 1:</label>
                    <input type="number" name="value1" placeholder="100" />
                    
                    <label>Label 2:</label>
                    <input type="text" name="label2" placeholder="Marketing" />
                    <label>Value 2:</label>
                    <input type="number" name="value2" placeholder="75" />
                    
                    <label>Label 3:</label>
                    <input type="text" name="label3" placeholder="Support" />
                    <label>Value 3:</label>
                    <input type="number" name="value3" placeholder="50" />
                </div>
                
                <div class="form-group">
                    <button type="submit">Generate Chart</button>
                </div>
            </form>
        </div>
        
        <p><a href="/">Back to Home</a></p>
    </div>
</body>
</html>""")

if __name__ == "__main__":
    main()