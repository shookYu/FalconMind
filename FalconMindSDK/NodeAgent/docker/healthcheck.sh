#!/bin/sh
# FalconMind NodeAgent Health Check

# Check if process is running
if ! pgrep -x "nodeagent" > /dev/null 2>&1; then
    echo "CRITICAL: NodeAgent process not running"
    exit 2
fi

# Check database connectivity
if [ -f /var/lib/nodeagent/offline.db ]; then
    if ! sqlite3 /var/lib/nodeagent/offline.db "SELECT 1;" > /dev/null 2>&1; then
        echo "WARNING: Database connectivity issue"
        exit 1
    fi
else
    echo "WARNING: Database not initialized"
fi

# Check log directory
if [ ! -d /var/log/nodeagent ]; then
    echo "WARNING: Log directory missing"
    exit 1
fi

# Check disk space
DISK_USAGE=$(df /var/lib/nodeagent | tail -1 | awk '{print $5}' | sed 's/%//')
if [ "$DISK_USAGE" -gt 90 ]; then
    echo "WARNING: Disk usage at ${DISK_USAGE}%"
    exit 1
fi

# All checks passed
echo "OK: NodeAgent healthy"
exit 0
