<?php
// Simple PHP info script
header('Content-Type: text/html');
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head>\n";
echo "  <title>PHP Test</title>\n";
echo "  <style>body { font-family: sans-serif; margin: 20px; }</style>\n";
echo "</head>\n";
echo "<body>\n";
echo "  <h1>PHP is working!</h1>\n";
echo "  <p>Current time: " . date('Y-m-d H:i:s') . "</p>\n";
echo "  <p>Server software: " . $_SERVER['SERVER_SOFTWARE'] . "</p>\n";
echo "  <h2>PHP Information</h2>\n";
echo "  <pre>\n";
echo "PHP Version: " . phpversion() . "\n";
echo "  </pre>\n";
echo "</body>\n";
echo "</html>";
?>