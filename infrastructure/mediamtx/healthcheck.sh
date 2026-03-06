#!/bin/bash
# MediaTx Health Check Script
# Production-grade health verification

set -e

# Configuration
API_URL="http://localhost:9997/v3/config/global/get"
RTSP_URL="rtsp://localhost:8554/live/camera"
TIMEOUT=5
RETRY_COUNT=3

# Function: Check API health
check_api() {
    local retry=0
    while [ $retry -lt $RETRY_COUNT ]; do
        if curl -sf --max-time $TIMEOUT "$API_URL" > /dev/null 2>&1; then
            echo "API check: OK"
            return 0
        fi
        retry=$((retry + 1))
        sleep 1
    done
    echo "API check: FAILED"
    return 1
}

# Function: Check RTSP endpoint (optional - may not have active streams)
check_rtsp() {
    # Use wget to check if RTSP port is open
    if wget --timeout=$TIMEOUT --tries=1 --spider "$RTSP_URL" 2>&1 | grep -q "200\|302\|401"; then
        echo "RTSP check: OK (endpoint exists)"
        return 0
    fi
    
    # If no stream, check if port is listening
    if netstat -tuln 2>/dev/null | grep -q ":8554 " || \
       ss -tuln 2>/dev/null | grep -q ":8554 "; then
        echo "RTSP check: OK (port listening)"
        return 0
    fi
    
    echo "RTSP check: FAILED"
    return 1
}

# Function: Check metrics endpoint
check_metrics() {
    if curl -sf --max-time $TIMEOUT "http://localhost:9998/metrics" > /dev/null 2>&1; then
        echo "Metrics check: OK"
        return 0
    fi
    echo "Metrics check: FAILED"
    return 1
}

# Function: Check disk space
check_disk() {
    local usage
    usage=$(df /var/lib/falconmind | tail -1 | awk '{print $5}' | sed 's/%//')
    if [ "$usage" -lt 90 ]; then
        echo "Disk check: OK (${usage}% used)"
        return 0
    fi
    echo "Disk check: WARNING (${usage}% used)"
    return 1
}

# Function: Check memory
check_memory() {
    local available
    available=$(free | grep Mem | awk '{print $7}')
    if [ "$available" -gt 100000 ]; then  # > 100MB available
        echo "Memory check: OK"
        return 0
    fi
    echo "Memory check: WARNING"
    return 1
}

# Main health check
main() {
    local exit_code=0
    
    echo "=== MediaTx Health Check ==="
    echo "Timestamp: $(date -Iseconds)"
    echo ""
    
    # Critical checks
    if ! check_api; then
        exit_code=1
    fi
    
    if ! check_metrics; then
        exit_code=1
    fi
    
    # Warning checks (don't fail health check)
    check_rtsp || true
    check_disk || true
    check_memory || true
    
    echo ""
    if [ $exit_code -eq 0 ]; then
        echo "Health check: PASSED"
    else
        echo "Health check: FAILED"
    fi
    
    return $exit_code
}

main "$@"
