# FalconMindBuilder Frontend

Vue 3 + TypeScript frontend for FalconMindBuilder - UAV edge-side visual development tool.

## Tech Stack

- **Framework**: Vue 3.4 + TypeScript
- **Build Tool**: Vite 5
- **State Management**: Pinia
- **UI Components**: Element Plus
- **Flow Editor**: @vue-flow/core
- **HTTP Client**: Axios
- **Routing**: Vue Router 4

## Quick Start

### 1. Install Dependencies

```bash
cd FalconMindBuilder/frontend
pnpm install
```

### 2. Start Dev Server

```bash
# Using start script
./start.sh

# Or manually
pnpm dev
```

### 3. Access Application

- **Frontend**: http://localhost:5173
- **Backend API**: http://localhost:8000 (proxied)

## Project Structure

```
frontend/
├── src/
│   ├── api/
│   │   ├── client.ts        # Axios client
│   │   ├── projects.ts      # Projects API
│   │   └── flows.ts         # Flows API
│   ├── components/          # Vue components
│   ├── views/
│   │   ├── HomeView.vue     # Home page
│   │   ├── ProjectView.vue  # Project detail
│   │   └── FlowEditorView.vue  # Flow editor
│   ├── router/
│   │   └── index.ts         # Vue Router config
│   ├── stores/              # Pinia stores
│   ├── types/               # TypeScript types
│   ├── utils/               # Utility functions
│   ├── App.vue              # Root component
│   └── main.ts              # Entry point
├── public/
├── index.html
├── package.json
├── vite.config.ts
├── tsconfig.json
└── start.sh
```

## Features

### ✅ Implemented

- Project management (list, create, delete)
- Flow management (list, create, update, delete)
- Visual flow editor with drag & drop
- Component library (triggers, actions, conditions)
- Properties panel
- Flow export to SDK format

### 🔜 TODO

- Node validation
- Undo/Redo
- Flow templates
- 3D preview (Cesium)
- Real-time telemetry
- Authentication

## API Integration

The frontend connects to the backend API at `/api`:

```typescript
// List projects
const projects = await projectsApi.list()

// Create flow
const flow = await flowsApi.create(projectId, {
  name: 'My Flow',
  nodes: [...],
  edges: [...]
})

// Export flow to SDK format
const exportData = await flowsApi.export(projectId, flowId)
```

## Build

```bash
# Build for production
pnpm build

# Preview production build
pnpm preview
```

## Environment Variables

Create `.env` file:

```bash
VITE_API_URL=/api
```

## License

Apache License 2.0
