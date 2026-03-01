#!/bin/sh
# FalconMind NodeAgent Status Report

echo "========================================"
echo "FalconMind NodeAgent Status"
echo "========================================"
echo "Timestamp: $(date -Iseconds)"
echo ""

# Process status
echo "Process Status:"
if pgrep -x "nodeagent" > /dev/null 2>&1; then
    PID=$(pgrep -x "nodeagent")
    echo "  Running (PID: $PID)"
    
    # Memory usage
    MEM=$(ps -o rss= -p "$PID" 2>/dev/null | awk '{printf "%.1f MB", $1/1024}')
    echo "  Memory: $MEM"
    
    # CPU usage
    CPU=$(ps -o %cpu= -p "$PID" 2>/dev/null | tr -d ' ')
    echo "  CPU: ${CPU}%"
else
    echo "  NOT RUNNING"
fi
echo ""

# Database status
echo "Database Status:"
if [ -f /var/lib/nodeagent/offline.db ]; then
    DB_SIZE=$(ls -lh /var/lib/nodeagent/offline.db | awk '{print $5}')
    echo "  File: /var/lib/nodeagent/offline.db"
    echo "  Size: $DB_SIZE"
    
    # Count records
    if command -v sqlite3 > /dev/null 2>&1; then
        TASKS=$(sqlite3 /var/lib/nodeagent/offline.db "SELECT COUNT(*) FROM offline_tasks;" 2>/dev/null || echo "0")
        TELEMETRY=$(sqlite3 /var/lib/nodeagent/offline.db "SELECT COUNT(*) FROM telemetry;" 2>/dev/null || echo "0")
        echo "  Tasks: $TASKS"
        echo "  Telemetry records: $TELEMETRY"
    fi
else
    echo "  Database not found"
fi
echo ""

# Log status
echo "Log Status:"
if [ -d /var/log/nodeagent ]; then
    LOG_COUNT=$(ls /var/log/nodeagent/*.log 2>/dev/null | wc -l)
    echo "  Log files: $LOG_COUNT"
    
    if [ "$LOG_COUNT" -gt 0 ]; then
        LATEST_LOG=$(ls -t /var/log/nodeagent/*.log | head -1)
        LOG_SIZE=$(ls -lh "$LATEST_LOG" | awk '{print $5}')
        echo "  Latest: $(basename "$LATEST_LOG") ($LOG_SIZE)"
    fi
else
    echo "  Log directory not found"
fi
echo ""

# Disk usage
echo "Disk Usage:"
df -h /var/lib/nodeagent | tail -1 | awk '{printf "  Used: %s/%s (%s)\n", $3, $2, $5}'
echo ""

# Configuration
echo "Configuration:"
if [ -f /etc/nodeagent/config.yaml ]; then
    UAV_ID=$(grep "id:" /etc/nodeagent/config.yaml | head -1 | awk '{print $2}' | tr -d '"')
    echo "  UAV ID: ${UAV_ID:-Unknown}"
else
    echo "  Config file not found"
fi
echo ""

# Recent log entries
echo "Recent Log Entries:"
if [ -d /var/log/nodeagent ] && [ "$(ls -A /var/log/nodeagent)" ]; then
    LATEST_LOG=$(ls -t /var/log/nodeagent/*.log | head -1)
    tail -5 "$LATEST_LOG" 2>/dev/null || echo "  (No recent entries)"
else
    echo "  (No logs available)"
fi
echo ""

echo "========================================"
