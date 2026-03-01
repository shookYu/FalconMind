#!/bin/sh
# FalconMind NodeAgent Docker Entrypoint

set -e

# Default configuration
UAV_ID=${UAV_ID:-"UAV_DOCKER_001"}
UAV_NAME=${UAV_NAME:-"DockerUAV"}
LOG_LEVEL=${LOG_LEVEL:-"INFO"}
DB_PATH=${DB_PATH:-"/var/lib/nodeagent/offline.db"}
METRICS_ENABLED=${METRICS_ENABLED:-"true"}
METRICS_PORT=${METRICS_PORT:-"9090"}

# Print startup info
echo "=========================================="
echo "FalconMind NodeAgent Starting"
echo "=========================================="
echo "UAV ID: $UAV_ID"
echo "UAV Name: $UAV_NAME"
echo "Log Level: $LOG_LEVEL"
echo "Database: $DB_PATH"
echo "Metrics: $METRICS_ENABLED (port $METRICS_PORT)"
echo "=========================================="

# Ensure data directories exist
mkdir -p /var/lib/nodeagent
mkdir -p /var/log/nodeagent

# Set permissions
chown -R nodeagent:nodeagent /var/lib/nodeagent
chown -R nodeagent:nodeagent /var/log/nodeagent

# Check if configuration file exists, create default if not
if [ ! -f /etc/nodeagent/config.yaml ]; then
    echo "Creating default configuration..."
    cat > /etc/nodeagent/config.yaml << EOF
uav:
  id: "${UAV_ID}"
  name: "${UAV_NAME}"
  type: "quadcopter"

gcs:
  host: "${GCS_HOST:-192.168.1.100}"
  port: ${GCS_PORT:-8080}
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000

swarm:
  enabled: true
  swarm_id: "${SWARM_ID:-SWARM_001}"
  discovery_interval_ms: 5000
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000

autonomy:
  enabled: true
  heartbeat_timeout_seconds: 10
  max_offline_duration_minutes: 30
  low_battery_threshold: 30
  critical_battery_threshold: 15

storage:
  db_path: "${DB_PATH}"
  max_telemetry_buffer: 1000

logging:
  level: "${LOG_LEVEL}"
  output: "/var/log/nodeagent"
  format: "json"

metrics:
  enabled: ${METRICS_ENABLED}
  export_interval_seconds: 30
  prometheus_port: ${METRICS_PORT}
EOF
fi

# Check serial ports
if [ -e /dev/ttyUSB0 ]; then
    echo "Found serial port: /dev/ttyUSB0"
fi

if [ -e /dev/ttyACM0 ]; then
    echo "Found serial port: /dev/ttyACM0"
fi

# Run the requested command
if [ "$1" = "nodeagent" ]; then
    echo "Starting NodeAgent..."
    exec nodeagent_demo "$@"
elif [ "$1" = "test" ]; then
    echo "Running tests..."
    exec nodeagent_unit_tests "$@"
elif [ "$1" = "shell" ] || [ "$1" = "/bin/sh" ]; then
    echo "Starting shell..."
    exec /bin/sh
else
    exec "$@"
fi
