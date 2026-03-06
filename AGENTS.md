# FalconMind - Agent Instructions

## Project Structure

| Component | Purpose | Tech Stack | Location |
|-----------|---------|------------|----------|
| **FalconMindSDK** | Core SDK (AI perception, mission planning, flight control) | C++17, CMake | `FalconMindSDK/` |
| **FalconMindBuilder** | Edge-side visual development tool | Vue 3, FastAPI, SQLite | `FalconMindBuilder/` |
| **FalconMindViewer** | Ground control platform | Vue 3, FastAPI, PostgreSQL | `FalconMindViewer/` |
| **NodeAgent** | Offline autonomy agent (P0/P1/P2) | C++17 | `FalconMindSDK/NodeAgent/` |

## Build Commands

### FalconMindSDK (C++)
```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DFALCONMINDSDK_BUILD_TESTS=ON
make -j4 && make install

# Cross-compile ARM64 (RK3588/RK3576)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake

# Python bindings
cmake .. -DFALCONMINDSDK_BUILD_PYTHON=ON
```

### NodeAgent
```bash
cd FalconMindSDK/NodeAgent/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNODEAGENT_STANDALONE=ON
make -j4
```

### FalconMindBuilder
```bash
cd FalconMindBuilder

# Docker (recommended)
docker-compose up -d

# Or native
cd backend && pip install -r requirements.txt && python main.py
cd frontend && pnpm install && pnpm dev
```

### FalconMindViewer
```bash
cd FalconMindViewer
./start-dev.sh
```

## Test Commands

### SDK Tests
```bash
cd FalconMindSDK/build

# Run all tests
ctest --output-on-failure

# Run single test by name
ctest -R falconmind_sdk_core_tests --output-on-failure

# Run test executable directly
./falconmind_sdk_core_tests
./falconmind_flow_executor_tests
```

### NodeAgent Tests
```bash
cd FalconMindSDK/NodeAgent/build
./nodeagent_unit_tests
./nodeagent_benchmarks
```

### Frontend Tests
```bash
cd FalconMindBuilder/frontend  # or FalconMindViewer/frontend
pnpm run test:unit      # vitest
pnpm run test:e2e       # playwright
```

### Viewer Backend Tests
```bash
cd FalconMindViewer/backend
pytest tests/
```

## Code Style & Formatting

### C++ (C++17)
- **Format**: `cd FalconMindSDK && ./format-code.sh format`
- **Line length**: 120 columns
- **Indent**: 4 spaces (spaces, never tabs)

Naming conventions:
| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `Pipeline`, `Node` |
| Functions | camelCase | `addNode()`, `process()` |
| Private Members | snake_case_ | `state_`, `nodes_` |
| Local Variables | snake_case | `config`, `result` |
| Constants | kPascalCase | `kMaxNodes` |
| Files | PascalCase.cpp | `Pipeline.cpp` |

Include order:
```cpp
// 1. Project headers
#include "falconmind/sdk/core/Pipeline.h"

// 2. Third-party headers
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

// 3. System headers
#include <iostream>
#include <memory>
```

### Python (3.8+)
- Use type hints (PEP 484)
- Functions: `snake_case`, Classes: `PascalCase`
- Return `Optional[T]` or use exceptions for exceptional cases

### TypeScript/Vue (ES2020, Vue 3.3+)
- **Lint**: `pnpm run lint` (ESLint + Prettier)
- **Format**: `pnpm run format`

Naming conventions:
| Type | Convention | Example |
|------|------------|---------|
| Components | PascalCase | `TargetSelector.vue` |
| Composables | camelCase | `useFlowExecutor()` |
| Types | PascalCase | `FlowConfig`, `NodeData` |
| Constants | UPPER_SNAKE_CASE | `API_BASE_URL` |

Import order: Vue/Vite → Third-party → Project

## Key Guidelines

### Memory Management (C++)
- **Always use smart pointers**: `std::shared_ptr<>`, `std::unique_ptr<>`
- **Never use raw `new`/`delete`**
- Use `std::move()` for transferring ownership

### Error Handling
- C++: Use `std::optional<T>` or `std::expected<T,E>` (C++23)
- Python: Return `Optional[T]` or raise exceptions
- **Never suppress type errors with `as any`, `@ts-ignore`, `@ts-expect-error`**

### Thread Safety
- Components are **not thread-safe unless explicitly documented**
- Use explicit synchronization (mutexes, locks) for shared state
- Document thread-safety guarantees in header comments

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `FALCONMINDSDK_BUILD_TESTS` | Build test programs | `ON` |
| `FALCONMINDSDK_BUILD_PYTHON` | Build Python bindings | `ON` |
| `FALCONMINDSDK_BUILD_NODEAGENT` | Build NodeAgent | `ON` |
| `FALCONMINDSDK_BUILD_RKNN_BACKEND` | RKNN backend for Rockchip | `ON` |
| `FALCONMINDSDK_BUILD_ROS2` | ROS2 integration | `OFF` |
| `FALCONMINDSDK_ENABLE_COVERAGE` | Coverage reporting | `OFF` |

## Quick Reference

### Run a Single Test File
```bash
# C++ (SDK)
./falconmind_sdk_core_tests
./falconmind_sdk_core_tests --gtest_filter=TestSuite.TestName

# C++ (NodeAgent)
./nodeagent_unit_tests

# Python
python test_gps_defender.py

# TypeScript/Vue
npx vitest run src/components/MyComponent.spec.ts
```

### Format Code
```bash
# C++
./format-code.sh format
./format-code.sh check

# Frontend
pnpm run format
```

### Coverage Report
```bash
cd FalconMindSDK
./coverage-report.sh html
```

## Documentation

- **SDK**: `FalconMindSDK/README.md`
- **NodeAgent**: `FalconMindSDK/NodeAgent/README.md`
- **Builder**: `FalconMindBuilder/Doc/`
- **Viewer**: `FalconMindViewer/docs/`
- **Main README**: `/home/shook/study/opencode/README.md`


<!-- open-mem-context -->
## Project Activity (auto-generated by open-mem)

### ./
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### docs/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/include/falconmind/sdk/flow/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/include/falconmind/sdk/mission/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/include/falconmind/sdk/perception/
| ID | Type | Title | Date |
|----|------|-------|------|
| b2559c98-5a22-4649-b3ab-af33a376824e | 🔵 discovery | Monocular Distance Estimator Header | 2026-03-06 |
| e877ec04-b552-4651-962b-0d02e89680b9 | 🔵 discovery | DeepSORT Tracker Backend Implementation | 2026-03-06 |
| ccde421e-975c-458d-92ad-a3cd775dd04b | 🔵 discovery | RKNN Detector Backend Skeleton | 2026-03-06 |

**Key concepts:** monocular-depth-estimation, camera-intrinsics, object-size-based-distance, falconmind-node, perception-pipeline, deepsort-tracking, kalman-filter, appearance-features, hungarian-algorithm, cascade-matching

### FalconMindSDK/python/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/python/tools/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/src/flow/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

### FalconMindSDK/tests/flow/
| ID | Type | Title | Date |
|----|------|-------|------|
| 410cbc3d-d3ab-456c-9677-d00556e14747 | 🟣 feature | FalconMindSDK Flow Node System Implementation | 2026-03-05 |

**Key concepts:** flow-based-architecture, mission-planning, modular-nodes, python-bindings, drone-autonomy, phase-2-completion

💡 *Use `mem-find` to search full details. Use `mem-create` to save important decisions.*
<!-- /open-mem-context -->
