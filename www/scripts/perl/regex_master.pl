#!/usr/bin/perl
use strict;
use warnings;
use CGI qw(:standard);

# Output Content-type
print "Content-type: text/html\n\n";

# Sample text for demonstrating regex capabilities
my $sample_text = <<'END_TEXT';
Welcome to the WebServ CGI demonstration with Perl v5.34.0!
This example showcases Perl's powerful regex capabilities.

Email examples:
  - john.doe@example.com
  - support@webserv-project.org
  - user123@mail.co.uk

IP Addresses:
  - 192.168.1.1
  - 10.0.0.255
  - 172.16.254.1
  
Dates in various formats:
  - 2025-05-23 (ISO)
  - 23/05/2025 (European)
  - 05/23/2025 (US)
  - May 23, 2025

Phone numbers:
  - (555) 123-4567
  - +1-555-123-4567
  - 555.123.4567
END_TEXT

# Get system info
my $perl_version = $];
my $os = $^O;
my $hostname = `hostname`;
chomp $hostname;
my $kernel = `uname -r`;
chomp $kernel;
my $uptime = `uptime -p`;
chomp $uptime;

# Perform regex operations
# Extract emails with regex
my @emails = $sample_text =~ /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b/g;

# Extract IP addresses
my @ips = $sample_text =~ /\b(?:\d{1,3}\.){3}\d{1,3}\b/g;

# Extract dates
my @dates = $sample_text =~ /\b\d{4}-\d{2}-\d{2}\b|\b\d{2}\/\d{2}\/\d{4}\b|\b[A-Z][a-z]+ \d{1,2}, \d{4}\b/g;

# Extract phone numbers
my @phones = $sample_text =~ /\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4}|\+\d-\d{3}-\d{3}-\d{4}/g;

# Demonstrate substitution with regex
my $highlighted_text = $sample_text;
$highlighted_text =~ s/(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b)/<span class="email">$1<\/span>/g;
$highlighted_text =~ s/(\b(?:\d{1,3}\.){3}\d{1,3}\b)/<span class="ip">$1<\/span>/g;
$highlighted_text =~ s/(\b\d{4}-\d{2}-\d{2}\b|\b\d{2}\/\d{2}\/\d{4}\b|\b[A-Z][a-z]+ \d{1,2}, \d{4}\b)/<span class="date">$1<\/span>/g;
$highlighted_text =~ s/(\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4}|\+\d-\d{3}-\d{3}-\d{4})/<span class="phone">$1<\/span>/g;

# Demonstrate pattern transformation
my $transformed_text = $sample_text;
# Convert email addresses to a anonymized format
$transformed_text =~ s/([A-Za-z0-9._%+-]+)@([A-Za-z0-9.-]+\.[A-Za-z]{2,})/$1\@[protected]/g;
# Redact IP addresses
$transformed_text =~ s/\b(?:\d{1,3}\.){3}\d{1,3}\b/xxx.xxx.xxx.xxx/g;

# Define some common regex patterns to display
my %regex_patterns = (
    'Email' => '\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b',
    'URL' => '\bhttps?:\/\/[A-Za-z0-9.-]+\.[A-Za-z]{2,}[^\s]*\b',
    'IP Address' => '\b(?:\d{1,3}\.){3}\d{1,3}\b',
    'ISO Date' => '\b\d{4}-\d{2}-\d{2}\b',
    'US Phone' => '\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4}',
    'Credit Card' => '\b(?:4[0-9]{12}(?:[0-9]{3})?|5[1-5][0-9]{14}|3[47][0-9]{13}|6(?:011|5[0-9]{2})[0-9]{12})\b',
    'HTML Tag' => '<([a-z][a-z0-9]*)\b[^>]*>(.*?)<\/\1>'
);

# Output HTML with embedded data elements for JavaScript to extract
my $html = <<HTML;
<div id="perl-data" style="display:none;">
    <div id="system-info">
        <div id="perl-version">$perl_version</div>
        <div id="os">$os</div>
        <div id="hostname">$hostname</div>
        <div id="kernel">$kernel</div>
        <div id="uptime">$uptime</div>
    </div>
    
    <div id="sample-text">$sample_text</div>
    <div id="highlighted-text">$highlighted_text</div>
    <div id="original-text">$sample_text</div>
    <div id="transformed-text">$transformed_text</div>
    
    <div id="extracted-data">
HTML

foreach my $email (@emails) {
    $html .= "<span class=\"email\">$email</span>";
}

$html .= '<div class="separator"></div>';

foreach my $ip (@ips) {
    $html .= "<span class=\"ip\">$ip</span>";
}

$html .= '<div class="separator"></div>';

foreach my $date (@dates) {
    $html .= "<span class=\"date\">$date</span>";
}

$html .= '<div class="separator"></div>';

foreach my $phone (@phones) {
    $html .= "<span class=\"phone\">$phone</span>";
}

$html .= <<HTML;
    </div>
    
    <div id="regex-patterns">
HTML

foreach my $name (sort keys %regex_patterns) {
    my $pattern = $regex_patterns{$name};
    $html .= <<HTML;
        <div class="pattern">
            <div class="pattern-name">$name</div>
            <div class="pattern-code">$pattern</div>
        </div>
HTML
}

$html .= <<HTML;
    </div>
</div>
HTML

print $html;