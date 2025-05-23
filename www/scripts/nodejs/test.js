#!/usr/bin/env node

console.log("Content-type: text/html\n");
console.log("<html>");
console.log("<head>");
console.log("<title>Node.js CGI Test</title>");
console.log("</head>");
console.log("<body>");
console.log("<h1>Node.js CGI Test</h1>");
console.log("<p>This is a test script for Node.js CGI support.</p>");
console.log("<h2>Environment Variables:</h2>");
console.log("<ul>");

// Sort environment variables for consistent output
const envVars = Object.keys(process.env).sort();
for (const key of envVars) {
  console.log(`<li>${key}: ${process.env[key]}</li>`);
}

console.log("</ul>");
console.log("</body>");
console.log("</html>");