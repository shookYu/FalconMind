# FalconMindSDK Agent Instructions

## Build Commands

### SDK Build
```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

### Example Build
```bash
cd FalconMindSDK/examples/01_pipeline_basic/x86
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

### Cross-Compilation (ARM64)
```bash
cd FalconMindSDK/build_arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-linux-gnu.cmake
make -j4
make install
```

### Run Tests
```bash
cd FalconMindSDK/build
ctest
./falconmind_sdk_core_tests
./falconmind_flow_executor_tests
```

### Run Single Example
```bash
cd FalconMindSDK/examples/01_pipeline_basic/x86/build
./01_pipeline_basic_x86
```

## Code Style Guidelines

### Language & Standards
- **C++ Standard**: C++17
- **Build System**: CMake 3.16+
- **Target Platforms**: x86_64, ARM64 (RK3588/RK3576/RV1126B)

### Naming Conventions
- **Classes**: PascalCase (`Pipeline`, `Node`, `Pad`)
- **Functions**: camelCase (`addNode()`, `process()`)
- **Private Members**: snake_case with underscore (`state_`, `nodes_`)
- **File Names**: PascalCase matching class names

### Include Style
```cpp
#include <iostream>
#include <memory>
#include "nlohmann/json.hpp"
#include "falconmind/sdk/core/Pipeline.h"
```

### Class Structure
```cpp
class Node {
public:
    explicit Node(const std::string& id);
    virtual ~Node() = default;
    std::string id() const;
    virtual void process() = 0;

protected:
    std::shared_ptr<Pad> getPad(const std::string& name);

private:
    std::string id_;
    std::vector<std::shared_ptr<Pad>> pads_;
};
```

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
│   ├── core/                  # Pipeline, Node, Pad, Bus
│   ├── perception/            # Detection, Tracking
│   ├── sensors/               # Camera, IMU, GNSS
│   ├── flight/                # Flight control
│   └── mission/               # Behavior trees
├── src/                       # Implementation
├── tests/                     # Unit tests
├── examples/                  # Example programs (01-41)
├── python/                    # Python bindings
└── NodeAgent/                 # NodeAgent subproject
```

## Testing Guidelines

- Tests use simple `assert()` statements
- Test executables named `falconmind_*_tests`
- Tests registered with CTest via `add_test()`
- Each major component has its own test file

## Backend Options

Enable optional inference backends via CMake:
- `FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND=ON`
- `FALCONMINDSDK_BUILD_RKNN_BACKEND=ON`
- `FALCONMINDSDK_BUILD_TENSORRT_BACKEND=ON`

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

## Dependencies

- nlohmann/json (JSON parsing)
- cpp-httplib (HTTP server for FlowExecutor)
- pybind11 (Python bindings, optional)
- ONNX Runtime (optional inference backend)
- RKNN Toolkit2 (optional for ARM64 boards)


## Installation Paths

- x86: `install/x86/` (lib/, include/)
- ARM64: `install/arm64/` (lib/, include/)
