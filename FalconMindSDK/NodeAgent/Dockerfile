# FalconMind NodeAgent Docker Image

## Overview

This Docker image packages the FalconMind NodeAgent for edge deployment on UAV companion computers (e.g., Raspberry Pi, NVIDIA Jetson, Rockchip boards).

## Features

- **Multi-arch support**: x86_64, ARM64, ARMv7
- **Offline autonomy**: Full P0-P2 feature set
- **Swarm coordination**: Inter-UAV communication
- **Predictive reconnection**: Connection quality monitoring
- **Lightweight**: Alpine Linux base, minimal footprint

## Building

### Build for current architecture
```bash
docker build -t falconmind-nodeagent:latest .
```

### Build for ARM64 (Raspberry Pi 4, Jetson)
```bash
docker buildx build --platform linux/arm64 -t falconmind-nodeagent:arm64 .
```

### Build for x86_64
```bash
docker buildx build --platform linux/amd64 -t falconmind-nodeagent:amd64 .
```

### Multi-platform build
```bash
docker buildx create --use
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 \
  -t falconmind-nodeagent:latest --push .
```

## Running

### Basic usage
```bash
docker run -d \
  --name nodeagent \
  --privileged \
  --network host \
  -v /dev:/dev \
  -v $(pwd)/config:/etc/nodeagent \
  -v $(pwd)/data:/var/lib/nodeagent \
  falconmind-nodeagent:latest
```

### With configuration
```bash
docker run -d \
  --name nodeagent \
  --privileged \
  --network host \
  -v /dev/ttyUSB0:/dev/ttyUSB0 \
  -v $(pwd)/config/nodeagent.yaml:/etc/nodeagent/config.yaml:ro \
  -v $(pwd)/data:/var/lib/nodeagent \
  --restart unless-stopped \
  falconmind-nodeagent:latest
```

### Development mode (with shell access)
```bash
docker run -it --rm \
  --privileged \
  --network host \
  -v $(pwd)/config:/etc/nodeagent \
  falconmind-nodeagent:latest /bin/sh
```

## Configuration

Create a `config/nodeagent.yaml` file:

```yaml
uav:
  id: "UAV_001"
  name: "Alpha"
  type: "quadcopter"

gcs:
  host: "192.168.1.100"
  port: 8080
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000

swarm:
  enabled: true
  swarm_id: "SWARM_001"
  discovery_interval_ms: 5000
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000
  leader_election_timeout_ms: 15000

autonomy:
  enabled: true
  heartbeat_timeout_seconds: 10
  max_offline_duration_minutes: 30
  low_battery_threshold: 30
  critical_battery_threshold: 15
  gcs_reconnect_attempts: 5

storage:
  db_path: "/var/lib/nodeagent/offline.db"
  max_telemetry_buffer: 1000
  max_task_history: 100

logging:
  level: "INFO"
  output: "/var/log/nodeagent"
  max_file_size: 100
  max_files: 10
  format: "json"

metrics:
  enabled: true
  export_interval_seconds: 30
  prometheus_port: 9090
```

## Volumes

- `/etc/nodeagent`: Configuration files
- `/var/lib/nodeagent`: SQLite database and persistent data
- `/var/log/nodeagent`: Log files
- `/dev`: Device access (serial ports, etc.)

## Environment Variables

- `UAV_ID`: UAV identifier (overrides config)
- `UAV_NAME`: UAV display name
- `GCS_HOST`: Ground control station IP
- `GCS_PORT`: Ground control station port
- `SWARM_ID`: Swarm identifier
- `LOG_LEVEL`: Logging level (DEBUG, INFO, WARN, ERROR)
- `DB_PATH`: SQLite database path
- `METRICS_ENABLED`: Enable Prometheus metrics (true/false)

## Health Check

The container includes a health check:

```bash
docker exec nodeagent nodeagent-health
```

Returns:
- `0`: Healthy
- `1`: Degraded (still functional)
- `2`: Unhealthy (restart recommended)

## Monitoring

### View logs
```bash
docker logs -f nodeagent
```

### View metrics
```bash
curl http://localhost:9090/metrics
```

### Check status
```bash
docker exec nodeagent nodeagent-status
```

## Troubleshooting

### Permission denied on serial port
```bash
# Add user to dialout group on host
sudo usermod -a -G dialout $USER

# Or run container with privileged mode (already default)
docker run --privileged ...
```

### Database locked
```bash
# Check if another process is using the database
docker exec nodeagent lsof /var/lib/nodeagent/offline.db

# Reset database if corrupted
docker exec nodeagent rm /var/lib/nodeagent/offline.db
docker restart nodeagent
```

### High memory usage
```bash
# Check memory limits
docker stats nodeagent

# Set memory limit
docker run -m 512m --memory-swap 512m ...
```

## Production Deployment

### Docker Compose

```yaml
version: '3.8'

services:
  nodeagent:
    image: falconmind-nodeagent:latest
    container_name: nodeagent
    privileged: true
    network_mode: host
    restart: unless-stopped
    volumes:
      - ./config:/etc/nodeagent:ro
      - ./data:/var/lib/nodeagent
      - ./logs:/var/log/nodeagent
      - /dev:/dev
    environment:
      - UAV_ID=${UAV_ID:-UAV_001}
      - LOG_LEVEL=${LOG_LEVEL:-INFO}
    logging:
      driver: "json-file"
      options:
        max-size: "100m"
        max-file: "5"
    healthcheck:
      test: ["CMD", "nodeagent-health"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s
```

### Kubernetes (for ground station)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nodeagent
spec:
  replicas: 1
  selector:
    matchLabels:
      app: nodeagent
  template:
    metadata:
      labels:
        app: nodeagent
    spec:
      containers:
      - name: nodeagent
        image: falconmind-nodeagent:latest
        ports:
        - containerPort: 9090
          name: metrics
        env:
        - name: UAV_ID
          value: "GCS_001"
        - name: LOG_LEVEL
          value: "INFO"
        volumeMounts:
        - name: config
          mountPath: /etc/nodeagent
          readOnly: true
        - name: data
          mountPath: /var/lib/nodeagent
        resources:
          requests:
            memory: "256Mi"
            cpu: "250m"
          limits:
            memory: "512Mi"
            cpu: "500m"
      volumes:
      - name: config
        configMap:
          name: nodeagent-config
      - name: data
        persistentVolumeClaim:
          claimName: nodeagent-data
```

## License

Apache License 2.0
