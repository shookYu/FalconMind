# FalconMindSDK Agent Instructions

## Build Commands

### SDK Build (x86)
```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

### SDK Build with Tests
```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DFALCONMINDSDK_BUILD_TESTS=ON
make -j4
```

### Cross-Compilation (ARM64)
```bash
cd FalconMindSDK/build_arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake
make -j4
make install
```

### NodeAgent (Standalone Mode)
```bash
# 1. Build SDK as shared library
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
make -j4

# 2. Build NodeAgent independently
cd ../NodeAgent/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNODEAGENT_STANDALONE=ON
make -j4
```

## Test Commands

### Run All Tests
```bash
cd FalconMindSDK/build
ctest --output-on-failure
```

### Run Single Test
```bash
cd FalconMindSDK/build
ctest -R falconmind_sdk_core_tests --output-on-failure
# OR run directly:
./falconmind_sdk_core_tests
```

### Test Executables (SDK)
| Test | Executable |
|------|------------|
| Core Tests | `falconmind_sdk_core_tests` |
| Flow Executor | `falconmind_flow_executor_tests` |
| Node Factory | `falconmind_node_factory_tests` |
| Detection | `falconmind_detection_packet_tests` |
| YOLO Pre/Post | `falconmind_yolo_prepost_tests` |
| Tracker | `falconmind_tracker_tests` |
| Pipeline Link | `falconmind_pipeline_link_tests` |
| E2E Tests | `falconmind_flow_executor_e2e_tests` |

### NodeAgent Tests
```bash
cd FalconMindSDK/NodeAgent/build
./nodeagent_unit_tests
./nodeagent_benchmarks
```

## Code Style & Formatting

### Format Code
```bash
cd FalconMindSDK
./format-code.sh format    # Format all files
./format-code.sh check     # Check formatting (CI mode)
```

### Coverage Report
```bash
cd FalconMindSDK
./coverage-report.sh html     # HTML report
./coverage-report.sh xml      # XML report (CI)
./coverage-report.sh summary  # Console summary
```

## Code Style Guidelines

### Language & Standards
- **C++ Standard**: C++17
- **Build System**: CMake 3.16+
- **Target Platforms**: x86_64, ARM64 (RK3588/RK3576/RV1126B)
- **Line Length**: 120 columns
- **Indent**: 4 spaces (no tabs)

### Naming Conventions
| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `Pipeline`, `Node`, `Pad` |
| Functions | camelCase | `addNode()`, `process()` |
| Private Members | snake_case with trailing underscore | `state_`, `nodes_` |
| Local Variables | snake_case | `config`, `result` |
| File Names | PascalCase matching class | `Pipeline.cpp`, `Node.h` |
| Constants | kPascalCase | `kMaxNodes`, `kDefaultTimeout` |
| Macros | UPPER_SNAKE_CASE | `FALCONMINDSDK_BUILD_TESTS` |

### Include Style (Priority Order)
```cpp
// 1. Project headers (priority 2)
#include "falconmind/sdk/core/Pipeline.h"

// 2. Third-party headers (priority 3)
#include "nlohmann/json.hpp"

// 3. System headers (priority 4)
#include <iostream>
#include <memory>
#include <vector>
```

### Class Structure
```cpp
class Pipeline {
public:
    explicit Pipeline(const PipelineConfig& cfg);
    virtual ~Pipeline() = default;
    std::string id() const;
    bool addNode(const std::shared_ptr<Node>& node);
    virtual void process() = 0;

protected:
    std::shared_ptr<Pad> getPad(const std::string& name);

private:
    std::string id_;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes_;
};
```

### Formatting Rules (from .clang-format)
- **Braces**: Attach style (same line)
- **Pointer Alignment**: Left (`Type* ptr`)
- **Template Declarations**: Always break before
- **Short Functions**: Inline only
- **Include Blocks**: Regroup and sort by priority

### Error Handling
- Use `std::optional<T>` for values that may not exist
- Use `std::shared_ptr<T>` for shared ownership
- Return `bool` for success/failure
- Prefer `assert()` for internal invariants
- Never suppress type errors with `as any`, `@ts-ignore`, `@ts-expect-error`

### Memory Management
- Use smart pointers: `std::shared_ptr<>`, `std::unique_ptr<>`
- Never use raw `new`/`delete`
- Pipeline owns Nodes via `shared_ptr`

### Thread Safety
- Components not thread-safe unless documented
- Use explicit synchronization for shared state
- Prefer message passing over shared state

### Platform-Specific Code
```cpp
#ifdef FALCONMINDSDK_CROSS_COMPILE_ARM64
    // ARM64 specific code
#else
    // x86 specific code
#endif
```

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `FALCONMINDSDK_BUILD_TESTS` | Build test programs | ON |
| `FALCONMINDSDK_BUILD_PYTHON` | Build Python bindings | ON |
| `FALCONMINDSDK_BUILD_NODEAGENT` | Build NodeAgent | ON |
| `FALCONMINDSDK_BUILD_RKNN_BACKEND` | RKNN backend for Rockchip | ON |
| `FALCONMINDSDK_BUILD_ROS2` | ROS2 integration | OFF |
| `FALCONMINDSDK_ENABLE_COVERAGE` | Coverage reporting | OFF |
| `FALCONMINDSDK_OFFLINE_BUILD` | Offline mode (no downloads) | OFF |

## Installation Paths
- x86: `install/x86/` (lib/, include/)
- ARM64: `install/arm64/` (lib/, include/)

## Common Patterns

### Creating a Pipeline
```cpp
PipelineConfig cfg{"pipeline_001", "My Pipeline"};
auto pipeline = std::make_shared<Pipeline>(cfg);
auto source = std::make_shared<SourceNode>("source");
auto sink = std::make_shared<SinkNode>("sink");
pipeline->addNode(source);
pipeline->addNode(sink);
pipeline->link("source", "out", "sink", "in");
```

### Node Factory Registration
```cpp
NodeFactory::registerNodeType("MyNode", [](const std::string& id, const void*) {
    return std::make_shared<MyNode>(id);
});
```

## Available Skills & Tools

### UI UX Pro Max Skill (User-Installed)
Location: `/home/shook/.config/opencode/skill/ui-ux-pro-max-skill/`

AI-powered design intelligence toolkit for professional UI/UX:

| Resource | Count |
|----------|-------|
| UI Styles | 67 (Glassmorphism, Brutalism, Neumorphism, etc.) |
| Color Palettes | 96 industry-specific palettes |
| Font Pairings | 57 typography combinations |
| Reasoning Rules | 100 industry-specific design rules |
| Tech Stacks | 13 (React, Vue, Tailwind, shadcn, etc.) |

**Usage:**
```bash
# Search design recommendations
python3 /home/shook/.config/opencode/skill/ui-ux-pro-max-skill/src/ui-ux-pro-max/scripts/search.py "glassmorphism" --domain style

# Generate complete design system
python3 /home/shook/.config/opencode/skill/ui-ux-pro-max-skill/src/ui-ux-pro-max/scripts/search.py "SaaS dashboard" --design-system -p "MyApp"
```

**Domains:** `product`, `style`, `typography`, `color`, `landing`, `chart`, `ux`

**Stacks:** `html-tailwind`, `react`, `nextjs`, `astro`, `vue`, `nuxtjs`, `shadcn`, `svelte`, `swiftui`, `react-native`, `flutter`, `jetpack-compose`

## Project Structure
```
FalconMindSDK/
├── include/falconmind/sdk/    # Public headers
│   ├── core/                  # Pipeline, Node, Pad, Bus, Caps
│   ├── perception/            # Detection, Tracking, Inference
│   ├── sensors/               # Camera, IMU, GNSS
│   ├── flight/                # MAVLink flight control
│   └── mission/               # Waypoints, Search patterns
├── src/                       # Implementation
├── tests/                     # Unit tests
├── NodeAgent/                 # Offline autonomy (P0/P1/P2)
│   ├── src/                   # C++ source (15,000+ lines)
│   ├── tests/                 # Unit tests (250+)
│   ├── docker/                # Docker deployment
│   └── systemd/               # Systemd deployment
├── examples/                  # Example programs (01-41+)
├── scenarios/                 # PoC scenario implementations
└── 3rd/                       # Third-party dependencies
```
