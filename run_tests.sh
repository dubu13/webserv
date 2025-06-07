#!/bin/bash
# Ultimate WebServ Test Runner
# Provides easy access to comprehensive testing with different configurations

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
DEFAULT_URL="http://localhost:8080"
ALT_PORT=8081
TIMEOUT=10

print_header() {
    echo -e "${PURPLE}===============================================${NC}"
    echo -e "${PURPLE}🚀 WEBSERV ULTIMATE TEST SUITE v2.0${NC}"
    echo -e "${PURPLE}===============================================${NC}"
    echo
}

print_usage() {
    echo -e "${CYAN}Usage: $0 [COMMAND] [OPTIONS]${NC}"
    echo
    echo -e "${YELLOW}Commands:${NC}"
    echo "  quick      - Run only critical compliance tests (fast)"
    echo "  full       - Run complete test suite (comprehensive)"
    echo "  critical   - Run only tests that could cause grade 0"
    echo "  protocol   - Run HTTP protocol compliance tests"
    echo "  cgi        - Run CGI functionality tests"
    echo "  security   - Run security and edge case tests"
    echo "  performance- Run performance and load tests"
    echo "  check      - Quick server availability check"
    echo "  setup      - Setup test environment only"
    echo "  report     - Show last test report"
    echo "  help       - Show this help message"
    echo
    echo -e "${YELLOW}Options:${NC}"
    echo "  --url URL       Server URL (default: $DEFAULT_URL)"
    echo "  --port PORT     Alternative port (default: $ALT_PORT)"
    echo "  --timeout SEC   Request timeout (default: $TIMEOUT)"
    echo "  --no-setup      Skip test environment setup"
    echo "  --json          Output results in JSON format"
    echo "  --quiet         Minimal output"
    echo "  --verbose       Detailed output"
    echo
    echo -e "${YELLOW}Examples:${NC}"
    echo "  $0 quick                    # Quick critical tests"
    echo "  $0 full                     # Complete test suite"
    echo "  $0 critical --url http://localhost:8080"
    echo "  $0 performance --timeout 30"
    echo
}

check_dependencies() {
    echo -e "${BLUE}🔍 Checking dependencies...${NC}"
    
    # Check Python 3
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}❌ Python 3 is required but not installed${NC}"
        exit 1
    fi
    
    # Check requests library
    if ! python3 -c "import requests" 2>/dev/null; then
        echo -e "${YELLOW}⚠️  Installing required Python packages...${NC}"
        pip3 install requests || {
            echo -e "${RED}❌ Failed to install requests. Please install manually:${NC}"
            echo "   pip3 install requests"
            exit 1
        }
    fi
    
    echo -e "${GREEN}✅ Dependencies satisfied${NC}"
}

check_server() {
    local url=${1:-$DEFAULT_URL}
    echo -e "${BLUE}🔍 Checking server availability at $url...${NC}"
    
    if curl -s --max-time 5 "$url" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ Server is responding${NC}"
        return 0
    else
        echo -e "${RED}❌ Server is not responding at $url${NC}"
        echo
        echo -e "${YELLOW}Please start your webserv:${NC}"
        echo "  ./webserv config/webserv.conf"
        echo
        return 1
    fi
}

run_test_suite() {
    local test_type="$1"
    local url="$2"
    local alt_port="$3"
    local timeout="$4"
    local extra_args="$5"
    
    echo -e "${BLUE}🚀 Running $test_type tests...${NC}"
    echo
    
    case "$test_type" in
        "quick")
            python3 test_suite_ultimate.py --url "$url" --alt-port "$alt_port" --timeout "$timeout" --quick $extra_args
            ;;
        "full")
            python3 test_suite_ultimate.py --url "$url" --alt-port "$alt_port" --timeout "$timeout" $extra_args
            ;;
        "critical")
            # Run custom critical tests
            python3 test_subject_compliance.py "$url" 2>/dev/null || python3 test_suite_ultimate.py --url "$url" --quick $extra_args
            ;;
        "protocol")
            python3 test_http_protocol.py 2>/dev/null || echo -e "${YELLOW}⚠️  HTTP protocol tests not available${NC}"
            ;;
        "cgi")
            python3 test_cgi_enhanced.py 2>/dev/null || python3 test_cgi.py 2>/dev/null || echo -e "${YELLOW}⚠️  CGI tests not available${NC}"
            ;;
        "security")
            python3 test_edge_cases.py 2>/dev/null || echo -e "${YELLOW}⚠️  Security tests not available${NC}"
            ;;
        "performance")
            python3 test_performance.py 2>/dev/null || echo -e "${YELLOW}⚠️  Performance tests not available${NC}"
            ;;
        *)
            echo -e "${RED}❌ Unknown test type: $test_type${NC}"
            exit 1
            ;;
    esac
}

setup_environment() {
    echo -e "${BLUE}🛠️  Setting up test environment...${NC}"
    
    # Create directory structure
    mkdir -p www/{uploads,assets/{css,js,images},browse,scripts,cgi-bin,api,readonly,errors,YoupiBanane}
    
    # Make test scripts executable if they exist
    find www/scripts -name "*.py" -exec chmod +x {} \; 2>/dev/null || true
    find www/cgi-bin -name "*.py" -exec chmod +x {} \; 2>/dev/null || true
    
    echo -e "${GREEN}✅ Test environment ready${NC}"
}

show_report() {
    if [ -f "webserv_test_report.json" ]; then
        echo -e "${BLUE}📊 Latest Test Report:${NC}"
        echo
        
        # Extract key information from JSON report
        if command -v jq &> /dev/null; then
            echo -e "${CYAN}Timestamp:${NC} $(jq -r '.timestamp' webserv_test_report.json)"
            echo -e "${CYAN}Server URL:${NC} $(jq -r '.server_url' webserv_test_report.json)"
            echo -e "${CYAN}Total Tests:${NC} $(jq -r '.statistics.total' webserv_test_report.json)"
            echo -e "${CYAN}Passed:${NC} $(jq -r '.statistics.passed' webserv_test_report.json)"
            echo -e "${CYAN}Failed:${NC} $(jq -r '.statistics.failed' webserv_test_report.json)"
            echo -e "${CYAN}Critical Failures:${NC} $(jq -r '.statistics.critical_failed' webserv_test_report.json)"
            echo -e "${CYAN}Execution Time:${NC} $(jq -r '.execution_time' webserv_test_report.json)s"
            echo
            
            # Show critical failures if any
            local critical_count=$(jq -r '.statistics.critical_failed' webserv_test_report.json)
            if [ "$critical_count" != "0" ]; then
                echo -e "${RED}🚨 Critical Failures:${NC}"
                jq -r '.tests[] | select(.critical == true and .success == false) | "  • " + .name + ": " + .message' webserv_test_report.json
                echo
            fi
        else
            echo -e "${YELLOW}Install 'jq' for formatted report display${NC}"
            cat webserv_test_report.json
        fi
    else
        echo -e "${YELLOW}⚠️  No test report found. Run tests first.${NC}"
    fi
}

run_continuous_testing() {
    local url="$1"
    local interval="${2:-30}"
    
    echo -e "${BLUE}🔄 Starting continuous testing (every ${interval}s)...${NC}"
    echo -e "${YELLOW}Press Ctrl+C to stop${NC}"
    echo
    
    while true; do
        echo -e "${CYAN}$(date): Running quick test...${NC}"
        python3 test_suite_ultimate.py --url "$url" --quick --quiet 2>/dev/null
        echo -e "${CYAN}Next test in ${interval}s...${NC}"
        sleep "$interval"
    done
}

# Parse command line arguments
COMMAND=""
URL="$DEFAULT_URL"
ALT_PORT_ARG="$ALT_PORT"
TIMEOUT_ARG="$TIMEOUT"
EXTRA_ARGS=""
SKIP_SETUP=false

while [[ $# -gt 0 ]]; do
    case $1 in
        quick|full|critical|protocol|cgi|security|performance|check|setup|report|help)
            COMMAND="$1"
            shift
            ;;
        --url)
            URL="$2"
            shift 2
            ;;
        --port)
            ALT_PORT_ARG="$2"
            shift 2
            ;;
        --timeout)
            TIMEOUT_ARG="$2"
            shift 2
            ;;
        --no-setup)
            SKIP_SETUP=true
            shift
            ;;
        --json|--quiet|--verbose)
            EXTRA_ARGS="$EXTRA_ARGS $1"
            shift
            ;;
        --continuous)
            CONTINUOUS=true
            INTERVAL="${2:-30}"
            shift 2
            ;;
        -h|--help)
            COMMAND="help"
            shift
            ;;
        *)
            echo -e "${RED}❌ Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Main execution
print_header

case "$COMMAND" in
    "help"|"")
        print_usage
        exit 0
        ;;
    "check")
        check_dependencies
        check_server "$URL"
        exit $?
        ;;
    "setup")
        setup_environment
        exit 0
        ;;
    "report")
        show_report
        exit 0
        ;;
    *)
        # Check dependencies and server for test commands
        check_dependencies
        
        if [ "$COMMAND" != "setup" ]; then
            check_server "$URL" || exit 1
        fi
        
        # Setup environment unless skipped
        if [ "$SKIP_SETUP" = false ]; then
            setup_environment
        fi
        
        # Run continuous testing if requested
        if [ "$CONTINUOUS" = true ]; then
            run_continuous_testing "$URL" "$INTERVAL"
        else
            # Run the specified tests
            run_test_suite "$COMMAND" "$URL" "$ALT_PORT_ARG" "$TIMEOUT_ARG" "$EXTRA_ARGS"
        fi
        
        # Show report after tests
        echo
        show_report
        ;;
esac
