#!/usr/bin/env node

// Node.js CGI script demonstrating asynchronous capabilities and modern JS features
// Output HTTP headers
console.log("Content-type: application/json\n");

// Simulating asynchronous operations with promises
async function fetchSystemInfo() {
    return new Promise(resolve => {
        setTimeout(() => {
            resolve({
                nodeVersion: process.version,
                platform: process.platform,
                arch: process.arch,
                cpuCores: require('os').cpus().length,
                totalMemory: Math.round(require('os').totalmem() / (1024 * 1024 * 1024)) + ' GB',
                freeMemory: Math.round(require('os').freemem() / (1024 * 1024 * 1024)) + ' GB',
                uptime: Math.round(require('os').uptime() / 3600) + ' hours'
            });
        }, 100);
    });
}

// Simulating API data fetching
async function fetchApiData() {
    return new Promise(resolve => {
        setTimeout(() => {
            resolve([
                { id: 1, name: "User Management API", status: "Online", response: "23ms" },
                { id: 2, name: "Authentication Service", status: "Online", response: "45ms" },
                { id: 3, name: "Data Processing API", status: "Degraded", response: "120ms" },
                { id: 4, name: "Notification Service", status: "Offline", response: "N/A" },
                { id: 5, name: "Analytics Engine", status: "Online", response: "67ms" }
            ]);
        }, 200);
    });
}

// Modern JS features demonstration
const technologies = [
    { name: "Promises", description: "Handle asynchronous operations elegantly" },
    { name: "Async/Await", description: "Syntactic sugar for working with promises" },
    { name: "Arrow Functions", description: "Concise syntax for writing function expressions" },
    { name: "Destructuring", description: "Extract multiple properties from objects or arrays" },
    { name: "Template Literals", description: "Enhanced way to work with strings" },
    { name: "Spread Operator", description: "Expand iterables into individual elements" },
    { name: "ES Modules", description: "Standardized module system for JavaScript" }
];

// Code examples to display
const codeExamples = [
    {
        title: "Async/Await",
        code: `async function getData() {
  try {
    const response = await fetch('https://api.example.com/data');
    const data = await response.json();
    return data;
  } catch (error) {
    console.error('Error fetching data:', error);
  }
}`
    },
    {
        title: "Promise Chaining",
        code: `fetchData()
  .then(response => response.json())
  .then(data => processData(data))
  .catch(error => console.error(error))
  .finally(() => console.log('Done!'));`
    },
    {
        title: "Destructuring & Spread",
        code: `// Object destructuring
const { name, age, ...rest } = person;

// Array destructuring
const [first, second, ...others] = items;

// Spread operator
const newArray = [...array1, ...array2];
const newObj = { ...obj1, ...obj2, newProp: 'value' };`
    }
];

// Main function to execute all async operations
async function main() {
    try {
        // Use Promise.all to run async operations concurrently
        const [systemInfo, apiData] = await Promise.all([
            fetchSystemInfo(),
            fetchApiData()
        ]);

        // Create output object
        const output = {
            systemInfo,
            apiData,
            technologies,
            codeExamples
        };

        // Output JSON data
        console.log(JSON.stringify(output, null, 2));
    } catch (error) {
        console.log(JSON.stringify({
            error: error.message
        }));
    }
}

// Execute main function
main().catch(error => console.error(error));