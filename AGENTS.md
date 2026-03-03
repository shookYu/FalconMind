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

### Run All Tests
```bash
cd FalconMindSDK/build
ctest --output-on-failure
```

### Run Single Test
```bash
cd FalconMindSDK/build
ctest -R falconmind_sdk_core_tests --output-on-failure
# OR run executable directly:
./falconmind_sdk_core_tests
```

### Run Test Executable Directly
```bash
cd FalconMindSDK/build
./falconmind_sdk_core_tests
./falconmind_flow_executor_tests
./falconmind_node_factory_tests
./falconmind_pipeline_link_tests
```

### Build Single Example
```bash
cd FalconMindSDK/examples/01_pipeline_basic/x86
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
./01_pipeline_basic_x86
```

## Code Style & Formatting

### Format Code
```bash
cd FalconMindSDK
./format-code.sh format    # Format all files
./format-code.sh check     # Check formatting (CI mode)
```

### Generate Coverage Report
```bash
cd FalconMindSDK
./coverage-report.sh html     # HTML report
./coverage-report.sh xml      # XML report
./coverage-report.sh summary  # Console summary
```

## Code Style Guidelines

### Language & Standards
- **C++ Standard**: C++17
- **Build System**: CMake 3.16+
- **Target Platforms**: x86_64, ARM64 (RK3588/RK3576/RV1126B)

### Naming Conventions
- **Classes**: PascalCase (`Pipeline`, `Node`, `Pad`)
- **Functions**: camelCase (`addNode()`, `process()`)
- **Private Members**: snake_case with trailing underscore (`state_`, `nodes_`)
- **File Names**: PascalCase matching class names
- **Constants**: kPascalCase (preferred)

### Include Style (Priority Order)
```cpp
// 1. Project headers (priority 2)
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"

// 2. System headers (priority 4)
#include <iostream>
#include <memory>
#include <unordered_map>

// 3. Third-party headers (priority 3 or 4)
#include "nlohmann/json.hpp"
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
- **Indent**: 4 spaces (no tabs)
- **Line length**: 120 columns
- **Braces**: Attach style (same line)
- **Pointer alignment**: Left (`Type* ptr`)
- **Template declarations**: Always break before
- **Short functions**: Inline only

### Error Handling
- Use `std::optional<T>` for values that may not exist
- Use `std::shared_ptr<T>` for shared ownership
- Return `bool` for success/failure
- Prefer `assert()` for internal invariants

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
│   ├── core_pipeline_tests.cpp
│   ├── test_flow_executor.cpp
│   ├── test_node_factory.cpp
│   └── ...
├── examples/                  # Example programs (01-41+)
├── scenarios/                 # PoC scenario implementations
├── python/                    # Python bindings
├── NodeAgent/                 # NodeAgent subproject
└── 3rd/                       # Third-party dependencies
```

## Test Commands Reference

| Test Name | Command |
|-----------|---------|
| Core Tests | `ctest -R falconmind_sdk_core_tests` |
| Flow Executor | `ctest -R falconmind_flow_executor_tests` |
| Node Factory | `ctest -R falconmind_node_factory_tests` |
| Detection | `ctest -R falconmind_detection_packet_tests` |
| YOLO Pre/Post | `ctest -R falconmind_yolo_prepost_tests` |
| Tracker | `ctest -R falconmind_tracker_tests` |
| Pipeline Link | `ctest -R falconmind_pipeline_link_tests` |
| E2E Tests | `ctest -R falconmind_flow_executor_e2e_tests` |

## Backend CMake Options

Enable optional inference backends:
- `FALCONMINDSDK_BUILD_RKNN_BACKEND=ON` - RKNN backend for Rockchip NPU (default: ON)
- `FALCONMINDSDK_BUILD_ROS2=ON` - ROS2 integration
- `FALCONMINDSDK_ENABLE_COVERAGE=ON` (for coverage)

## Installation Paths

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
