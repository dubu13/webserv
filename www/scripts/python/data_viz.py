#!/usr/bin/env python3
import json
import sys
import os
import random
from datetime import datetime, timedelta

print("Content-type: application/json\n")

try:
    import numpy as np
    import statistics
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

# Generate current system info
system_info = {
    "time": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    "python_version": sys.version.split()[0],
    "hostname": os.popen('hostname').read().strip() or "webserv",
    "cpu_cores": os.cpu_count() or 4,
    "platform": sys.platform
}

# Generate sample data
if HAS_NUMPY:
    # Use numpy for better random number generation
    np.random.seed(42)
    data = np.random.normal(100, 20, size=200).tolist()
    
    # Calculate statistics
    stats = {
        "mean": float(np.mean(data)),
        "median": float(np.median(data)),
        "std": float(np.std(data)),
        "min": float(np.min(data)),
        "max": float(np.max(data)),
        "count": len(data)
    }
    
    # Create histogram data
    hist, bin_edges = np.histogram(data, bins=20)
    histogram = {
        "labels": [round((bin_edges[i] + bin_edges[i+1])/2) for i in range(len(bin_edges)-1)],
        "values": hist.tolist()
    }
    
    # Create category data
    categories = []
    cat_names = ['Category A', 'Category B', 'Category C']
    cat_values = np.random.randint(50, 150, size=3)
    total = sum(cat_values)
    
    for i, name in enumerate(cat_names):
        categories.append({
            "name": name,
            "value": int(cat_values[i]),
            "percentage": (cat_values[i] / total) * 100
        })
else:
    # Fallback to simple random data if numpy is not available
    random.seed(42)
    data = [random.gauss(100, 20) for _ in range(200)]
    
    # Calculate basic statistics
    data_mean = sum(data) / len(data)
    data_sorted = sorted(data)
    data_median = data_sorted[len(data) // 2]
    data_std = (sum((x - data_mean) ** 2 for x in data) / len(data)) ** 0.5
    
    stats = {
        "mean": data_mean,
        "median": data_median,
        "std": data_std,
        "min": min(data),
        "max": max(data),
        "count": len(data)
    }
    
    # Create simple histogram
    min_val = min(data)
    max_val = max(data)
    bin_width = (max_val - min_val) / 20
    bins = [0] * 20
    labels = [round(min_val + i * bin_width + bin_width / 2) for i in range(20)]
    
    for val in data:
        bin_idx = min(int((val - min_val) / bin_width), 19)
        bins[bin_idx] += 1
    
    histogram = {
        "labels": labels,
        "values": bins
    }
    
    # Create category data
    categories = []
    cat_names = ['Category A', 'Category B', 'Category C']
    cat_values = [random.randint(50, 150) for _ in range(3)]
    total = sum(cat_values)
    
    for i, name in enumerate(cat_names):
        categories.append({
            "name": name,
            "value": cat_values[i],
            "percentage": (cat_values[i] / total) * 100
        })

# Format sample data for display
sample_data_str = "\n".join([f"{i+1:<3} {val:.2f}" for i, val in enumerate(data[:10])])
sample_data_str += "\n... (total " + str(len(data)) + " values)"

# Create final JSON output
output = {
    "system_info": system_info,
    "stats": stats,
    "histogram": histogram,
    "categories": categories,
    "sample_data": sample_data_str,
    "has_numpy": HAS_NUMPY
}

# Return JSON data
print(json.dumps(output, indent=2))