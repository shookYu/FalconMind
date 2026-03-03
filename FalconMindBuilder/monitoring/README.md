# FalconMindBuilder Observability Stack

Complete monitoring and observability solution using Prometheus, Grafana, AlertManager, and Loki.

## 📊 Components

| Component | Port | Description |
|-----------|------|-------------|
| Prometheus | 9090 | Metrics collection and alerting |
| Grafana | 3000 | Visualization dashboards |
| AlertManager | 9093 | Alert routing and notification |
| Loki | 3100 | Log aggregation |
| Node Exporter | 9100 | System metrics |
| cAdvisor | 8080 | Container metrics |

## 🚀 Quick Start

### Start the Monitoring Stack

```bash
cd FalconMindBuilder

# Start observability stack
docker-compose -f docker-compose.monitoring.yml up -d

# Check status
docker-compose -f docker-compose.monitoring.yml ps
```

### Access the Services

- **Grafana**: http://localhost:3000 (admin/admin)
- **Prometheus**: http://localhost:9090
- **AlertManager**: http://localhost:9093

## 📈 Metrics Endpoints

### Backend API Metrics

The Builder backend exposes metrics at:

```
GET /metrics          # Prometheus metrics
GET /health           # Liveness probe
GET /ready            # Readiness probe
GET /version          # Version information
```

### Key Metrics

#### HTTP Metrics
- `http_requests_total` - Total HTTP requests
- `http_request_duration_seconds` - Request latency
- `http_requests_in_progress` - Active requests

#### Business Metrics
- `flow_operations_total` - Flow CRUD operations
- `project_operations_total` - Project operations
- `uav_deployments_total` - UAV deployments
- `uav_connection_status` - UAV online/offline status
- `active_deployments` - Current active deployments

#### System Metrics
- `system_cpu_usage_percent` - CPU usage
- `system_memory_usage_bytes` - Memory usage
- `db_query_duration_seconds` - Database query latency

## 🎯 Dashboards

### 1. System Overview
- Request rate, error rate, latency
- Flow and Project operations
- System resource usage

### 2. UAV Fleet Monitor
- UAV connection status
- Battery levels
- Deployment status
- Flight metrics

## 🔔 Alerts

### Critical Alerts
- High error rate (>5%)
- Service down
- Database connection error
- UAV low battery (<20%)

### Warning Alerts
- High latency (>1s P95)
- High CPU usage (>80%)
- High memory usage (>85%)
- UAV offline
- Deployment failures

## 🔧 Configuration

### Prometheus
Edit `monitoring/prometheus/prometheus.yml` to:
- Add new scrape targets
- Change retention period
- Configure remote storage

### AlertManager
Edit `monitoring/alertmanager/alertmanager.yml` to:
- Configure notification channels (email, Slack, PagerDuty)
- Set up routing rules
- Define inhibition rules

### Grafana
- Dashboards are auto-provisioned from `monitoring/grafana/dashboards/`
- Data sources are configured in `monitoring/grafana/provisioning/`

## 📊 Adding Custom Metrics

### From Python Code

```python
from app.core.monitoring import (
    record_flow_operation,
    record_uav_deployment,
    update_uav_connection_status
)

# Record a business operation
record_flow_operation('create', 'success')

# Record UAV deployment
record_uav_deployment('success')

# Update UAV status
update_uav_connection_status('UAV_001', 'Drone 1', True)
```

### Using Decorators

```python
from app.core.monitoring import timed, DB_QUERY_DURATION

@timed(DB_QUERY_DURATION, labels={'operation': 'select'})
def get_flows():
    return db.query(Flow).all()
```

## 📝 Logging

Structured logging with request ID correlation:

```python
from app.core.monitoring import get_logger

logger = get_logger(__name__)
logger.info("Processing flow", extra={'flow_id': flow_id})
```

Logs include:
- Timestamp
- Log level
- Request ID
- Message

View logs in Grafana via the Loki data source.

## 🐛 Troubleshooting

### No metrics in Prometheus
1. Check backend is running: `curl http://localhost:8000/metrics`
2. Verify Prometheus targets: http://localhost:9090/targets
3. Check network connectivity

### Alerts not firing
1. Check AlertManager status: http://localhost:9093
2. Verify alert rules: http://localhost:9090/rules
3. Test alert expression in Prometheus

### Grafana no data
1. Check data source configuration
2. Verify Prometheus is accessible from Grafana
3. Check dashboard queries

## 📚 Useful Queries

### Request Rate by Endpoint
```promql
sum(rate(http_requests_total[5m])) by (endpoint)
```

### Error Rate
```promql
sum(rate(http_requests_total{status_code=~"5.."}[5m])) 
/ 
sum(rate(http_requests_total[5m]))
```

### P95 Latency
```promql
histogram_quantile(0.95, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
)
```

### UAVs Online
```promql
sum(uav_connection_status)
```

### Recent Logs
```
{job="builder-backend"} |= "error"
```

## 🔒 Security

In production:
1. Change default Grafana password
2. Enable HTTPS for all endpoints
3. Use authentication for Prometheus/AlertManager
4. Restrict network access
5. Enable audit logging

## 📖 References

- [Prometheus Docs](https://prometheus.io/docs/)
- [Grafana Docs](https://grafana.com/docs/)
- [AlertManager Docs](https://prometheus.io/docs/alerting/latest/alertmanager/)
- [Loki Docs](https://grafana.com/docs/loki/latest/)
