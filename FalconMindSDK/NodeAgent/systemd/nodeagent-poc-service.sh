#!/bin/bash
### BEGIN INIT INFO
# Provides:          nodeagent
# Required-Start:    $network $syslog
# Required-Stop:     $network $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: FalconMind NodeAgent UAV Autonomy Service
# Description:       Offline autonomy agent for FalconMind UAV systems
### END INIT INFO

# NodeAgent Service Control Script
# Production-ready init script for NodeAgent

NAME="nodeagent"
DAEMON="/opt/falconmind/bin/nodeagent"
CONFIG="/etc/falconmind/nodeagent_poc_scenario_01.yaml"
PIDFILE="/var/run/nodeagent.pid"
LOGFILE="/var/log/nodeagent/nodeagent.log"

# User to run as (should have access to serial ports and UAV hardware)
USER="falconmind"
GROUP="falconmind"

# Check if daemon exists
if [ ! -x "$DAEMON" ]; then
    echo "Error: $DAEMON not found or not executable"
    exit 1
fi

# Create log directory if needed
mkdir -p "$(dirname "$LOGFILE")"
chown $USER:$GROUP "$(dirname "$LOGFILE")"

start() {
    echo "Starting $NAME..."
    
    if [ -f "$PIDFILE" ] && kill -0 $(cat "$PIDFILE") 2>/dev/null; then
        echo "$NAME is already running"
        return 1
    fi
    
    # Start NodeAgent
    start-stop-daemon --start \
        --background \
        --make-pidfile \
        --pidfile "$PIDFILE" \
        --chuid $USER:$GROUP \
        --exec "$DAEMON" \
        -- \
        --config "$CONFIG" \
        >> "$LOGFILE" 2>&1
    
    if [ $? -eq 0 ]; then
        echo "$NAME started successfully"
        return 0
    else
        echo "Failed to start $NAME"
        return 1
    fi
}

stop() {
    echo "Stopping $NAME..."
    
    if [ ! -f "$PIDFILE" ]; then
        echo "$NAME is not running"
        return 1
    fi
    
    start-stop-daemon --stop \
        --pidfile "$PIDFILE" \
        --retry=TERM/30/KILL/5
    
    rm -f "$PIDFILE"
    echo "$NAME stopped"
    return 0
}

restart() {
    stop
    sleep 2
    start
}

status() {
    if [ -f "$PIDFILE" ] && kill -0 $(cat "$PIDFILE") 2>/dev/null; then
        echo "$NAME is running (PID: $(cat $PIDFILE))"
        return 0
    else
        echo "$NAME is not running"
        return 1
    fi
}

# Parse command
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

exit 0
