#!/bin/bash

# Output HTTP headers with JSON content type
echo "Content-type: application/json"
echo ""

# Get system info
HOSTNAME=$(hostname)
KERNEL=$(uname -r)
UPTIME=$(uptime -p)
OS=$(grep PRETTY_NAME /etc/os-release | cut -d '"' -f 2)

# Get memory info
MEMORY_TOTAL=$(free -h | grep "Mem:" | awk '{print $2}')
MEMORY_USED=$(free -h | grep "Mem:" | awk '{print $3}')
MEMORY_FREE=$(free -h | grep "Mem:" | awk '{print $4}')
MEMORY_INFO="Total: ${MEMORY_TOTAL} | Used: ${MEMORY_USED} | Free: ${MEMORY_FREE}"

# Get disk info
DISK_INFO=$(df -h / | tail -n 1)
DISK_USAGE=$(echo "$DISK_INFO" | awk '{print $5}')
DISK_FREE=$(echo "$DISK_INFO" | awk '{print $4}')
DISK_INFO="Usage: ${DISK_USAGE} | Free Space: ${DISK_FREE}"

# Get top CPU processes
CPU_PROCESSES=$(ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -n 6)

# Get top memory processes
MEM_PROCESSES=$(ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%mem | head -n 6)

# Get network interfaces
NETWORK_INFO=$(ip -br addr show 2>/dev/null | grep -v "^lo")
if [ -z "$NETWORK_INFO" ]; then
    NETWORK_INFO="No network interfaces found"
fi

# Get running services
SERVICES=$(systemctl list-units --type=service --state=running 2>/dev/null | head -n 11 | tail -n 10)
if [ -z "$SERVICES" ]; then
    SERVICES="No service information available"
fi

# Create JSON output
cat << JSON
{
    "system_info": {
        "hostname": "${HOSTNAME}",
        "os": "${OS}",
        "kernel": "${KERNEL}",
        "uptime": "${UPTIME}"
    },
    "resources": {
        "memory": "${MEMORY_INFO}",
        "disk": "${DISK_INFO}"
    },
    "processes": {
        "cpu": $(printf '%s' "${CPU_PROCESSES}" | jq -R -s -c 'split("\n")[:-1]'),
        "memory": $(printf '%s' "${MEM_PROCESSES}" | jq -R -s -c 'split("\n")[:-1]')
    },
    "network": $(printf '%s' "${NETWORK_INFO}" | jq -R -s -c 'split("\n")[:-1]'),
    "services": $(printf '%s' "${SERVICES}" | jq -R -s -c 'split("\n")[:-1]')
}
JSON