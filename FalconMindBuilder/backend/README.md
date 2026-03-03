# FalconMindBuilder Backend

FastAPI backend for FalconMindBuilder - UAV edge-side visual development tool.

## Features

- ✅ Project management (CRUD)
- ✅ Flow management (CRUD)
- ✅ Flow export to SDK FlowExecutor format
- ✅ SQLite database
- ✅ REST API with OpenAPI documentation
- 🔜 SDK FlowExecutor integration
- 🔜 MQTT communication with NodeAgent

## Quick Start

### 1. Install Dependencies

```bash
cd FalconMindBuilder/backend
python3 -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt
```

### 2. Start Server

```bash
# Using start script
./start.sh

# Or manually
python -m uvicorn app.main:app --reload
```

### 3. Access API

- **API**: http://localhost:8000
- **Docs**: http://localhost:8000/docs
- **Health**: http://localhost:8000/health

## API Endpoints

### Projects

```bash
# List projects
GET /api/projects/

# Create project
POST /api/projects/
{
  "name": "My Project",
  "description": "Project description",
  "uav_id": "UAV_001"
}

# Get project
GET /api/projects/{project_id}

# Update project
PUT /api/projects/{project_id}

# Delete project
DELETE /api/projects/{project_id}
```

### Flows

```bash
# List flows
GET /api/projects/{project_id}/flows/

# Create flow
POST /api/projects/{project_id}/flows/
{
  "name": "My Flow",
  "nodes": [...],
  "edges": [...]
}

# Get flow
GET /api/projects/{project_id}/flows/{flow_id}

# Update flow
PUT /api/projects/{project_id}/flows/{flow_id}

# Delete flow
DELETE /api/projects/{project_id}/flows/{flow_id}

# Export flow to SDK format
GET /api/projects/{project_id}/flows/{flow_id}/export
```

## Testing

```bash
# Start server first
./start.sh

# In another terminal
python test_backend.py
```

## Project Structure

```
backend/
├── app/
│   ├── __init__.py
│   ├── main.py              # FastAPI application
│   ├── core/
│   │   ├── config.py        # Configuration
│   │   └── database.py      # Database setup
│   ├── api/
│   │   ├── projects.py      # Project routes
│   │   └── flows.py         # Flow routes
│   ├── models/
│   │   ├── project.py       # Project model
│   │   └── flow.py          # Flow model
│   ├── schemas/
│   │   ├── project.py       # Project schemas
│   │   └── flow.py          # Flow schemas
│   └── services/
│       └── sdk_service.py   # SDK integration
├── tests/
├── requirements.txt
├── start.sh
└── test_backend.py
```

## Configuration

Create `.env` file or set environment variables:

```bash
# Application
APP_NAME=FalconMindBuilder
DEBUG=true

# Server
HOST=0.0.0.0
PORT=8000

# Database
DATABASE_URL=sqlite:///./builder.db

# SDK
SDK_PATH=/opt/falconmind/sdk
SDK_ENABLED=false  # Set to true when SDK is available

# MQTT
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_ENABLED=false
```

## SDK Integration

To enable SDK FlowExecutor integration:

1. Set `SDK_ENABLED=true` in `.env`
2. Ensure SDK is installed at `/opt/falconmind/sdk`
3. The service will save flow definitions to `{SDK_PATH}/flows/`

## Next Steps

- [ ] Implement SDK C API binding (ctypes)
- [ ] Add MQTT service for NodeAgent communication
- [ ] Implement flow validation
- [ ] Add authentication/authorization
- [ ] Add flow templates
- [ ] Add real-time status updates

## License

Apache License 2.0
