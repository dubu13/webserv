#!/usr/bin/perl

print "Content-type: text/html\n\n";
print "<html>\n";
print "<head>\n";
print "<title>Perl CGI Test</title>\n";
print "</head>\n";
print "<body>\n";
print "<h1>Perl CGI Test</h1>\n";
print "<p>This is a test script for Perl CGI support.</p>\n";
print "<h2>Environment Variables:</h2>\n";
print "<ul>\n";

foreach $key (sort keys %ENV) {
    print "<li>$key: $ENV{$key}</li>\n";
}

print "</ul>\n";
print "</body>\n";
print "</html>\n";