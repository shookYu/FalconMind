# SDK 依赖库管理说明

## 概述

FalconMindSDK 依赖以下外部库：
- **nlohmann/json** - JSON解析库
- **cpp-httplib** - HTTP客户端库（用于FlowExecutor的loadFlowFromBuilder功能）

## 依赖库获取策略

CMakeLists.txt 已配置为：

1. **优先使用系统已安装的库**：通过 `find_package` 查找
2. **如果找不到则自动下载**：使用 FetchContent 下载到 `build/_deps/` 目录
3. **自动缓存机制**：FetchContent 会自动缓存已下载的库，**不会重复下载**

## 避免重复下载

### FetchContent 缓存机制

FetchContent 会自动将下载的库缓存到 `3rd/` 目录：
- 第一次编译：会下载依赖库到 `3rd/` 目录
- 后续编译：如果 `3rd` 目录已存在，**不会重新下载**
- 只有删除 `3rd` 目录后才会重新下载

### 使用 GIT_SHALLOW

已启用 `GIT_SHALLOW TRUE`，只下载最新提交，不下载完整 Git 历史，**大幅减少下载时间**。

## 推荐做法

### 方案1：使用系统安装的库（推荐）

**优点**：
- 无需下载，编译最快
- 系统统一管理，便于更新

**安装方法**：

```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# 或使用 vcpkg
vcpkg install nlohmann-json cpp-httplib
```

编译时会自动检测并使用系统库，无需下载。

### 方案2：使用 FetchContent 自动下载

**优点**：
- 无需手动安装
- 自动管理版本
- 有缓存机制，不会重复下载

**使用方法**：

```bash
cd FalconMindSDK
mkdir -p build && cd build
cmake ..  # 第一次会下载依赖库
make      # 后续编译不会重新下载
```

## 依赖库位置

### 系统安装
- nlohmann/json: `/usr/include/nlohmann/json.hpp` 或通过 pkg-config
- cpp-httplib: 通常通过 vcpkg 或手动安装

### FetchContent 下载位置
- nlohmann/json: `3rd/json-src/`
- cpp-httplib: `3rd/httplib-src/`

## 清理和重新下载

### 查看依赖库状态

```bash
cd FalconMindSDK
ls -la 3rd/  # 查看已下载的依赖库
```

### 强制重新下载

如果需要强制重新下载（例如更新版本）：

```bash
cd FalconMindSDK
rm -rf 3rd   # 删除缓存
cd build
cmake ..     # 重新下载
```

### 清理所有构建文件

```bash
cd FalconMindSDK
rm -rf 3rd   # 删除依赖库缓存
cd build
rm -rf *     # 删除所有构建文件
cmake ..     # 重新配置和下载
```

## 编译时间优化

### 第一次编译
- 如果使用 FetchContent：需要下载依赖库（约 1-2 分钟）
- 如果使用系统库：无需下载，直接编译

### 后续编译
- **不会重新下载**（除非删除 `_deps` 目录）
- 只编译修改的文件，速度很快

## 常见问题

### Q: 为什么每次编译都要下载依赖库？

**A**: 这通常是因为：
1. 每次编译都删除了 `3rd` 目录
2. 或者 `3rd` 目录被意外删除

**解决方案**：
- 保留 `3rd` 目录，只运行 `make` 重新编译
- 或者安装系统库，避免下载

### Q: 如何加快首次编译速度？

**A**: 
1. 安装系统库（推荐）
2. 或使用 `GIT_SHALLOW TRUE`（已启用，只下载最新提交）

### Q: 如何更新依赖库版本？

**A**: 
1. 修改 CMakeLists.txt 中的 `GIT_TAG`
2. 删除 `3rd` 目录中对应的库目录（如 `3rd/json-src/`）
3. 重新运行 `cmake ..`

## 验证依赖库

编译时会显示使用的库来源：

```
-- nlohmann/json: Using system installation
-- cpp-httplib: Using FetchContent (cached in .../3rd/httplib-src)
```

## 相关文件

- `CMakeLists.txt` - CMake 配置文件
- `CMakeLists_dependency_notes.txt` - 依赖库管理说明

---

## 推理后端与检测模型（主平台：RK）

**应用主平台**：**RK1126B、RK3576、RK3588**。推理以 **RKNN** 为主；**整体暂不考虑 ONNXRuntime、TensorRT**（仅保留接口与可选 stub，不作为主路径实现与验证）。

SDK 提供检测后端接口与多种实现骨架，默认不链接任何推理库，保证在无 NPU 环境也可编译。在主平台上需真实推理时，应启用 **RKNN** 并安装板端 RKNN 运行时。

### CMake 选项（默认均为 OFF）

| 选项 | 说明 | 主平台策略 |
|------|------|------------|
| `FALCONMINDSDK_BUILD_RKNN_BACKEND` | 启用 RKNN 后端（需板端 librknnrt.so / RKNN-Toolkit2） | **主路径**，RK1126B/RK3576/RK3588 必选 |
| `FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND` | 启用 ONNXRuntime 后端 | 暂不考虑 |
| `FALCONMINDSDK_BUILD_TENSORRT_BACKEND` | 启用 TensorRT 后端 | 暂不考虑 |

开启 `FALCONMINDSDK_BUILD_RKNN_BACKEND` 后，CMake 会通过 `RKNN_SDK_ROOT`（或默认 `/usr/local`、`/opt/rknn`）查找 `include/rknn_api.h` 与 `librknnrt.so`；未找到时 WARNING 并保持骨架，找到则自动链接并启用真实推理。

### RKNN 与模型（主平台）

- 使用 **RKNN-Toolkit2** 将 YOLO 等模型从 ONNX 转为 `.rknn`，并按芯片（RK1126B/RK3576/RK3588）选择对应量化/配置。
- 将 `.rknn` 放到目标路径（如 `/opt/models/yolo_v26_640.rknn`），在 `detectors_demo.yaml` 中设置 `model_path`、`label_path`（类别标签）。
- 详见 `Doc/MODEL_ZOO.md`、`demo/README_detector_config_demo.md` 与 `Doc/SDK_UNIMPLEMENTED.md`。

---

## SDK ↔ 算法容器（PRD 8.6）

SDK 通过 **ISlamServiceClient** 与算法容器 SLAM 服务对接，供 VisualSlamNode、LidarSlamNode 注入使用：

| 实现 | 说明 |
|------|------|
| **SlamServiceClientStub** | 不连接，`isAvailable()` 为 false，用于无容器场景。 |
| **SlamServiceClientFromFile** | 从指定文件读取位姿（二进制 Pose3D 布局）；算法容器将当前位姿写入该文件即可对接。 |
| **gRPC 客户端** | 可选。proto 见 `proto/slam_service.proto`；需自行用 protoc + grpc_cpp_plugin 生成 C++ 并链接 gRPC，实现 `SLAMService.GetPose()` 调用。 |

---

## 运行测试

编译时启用 `FALCONMINDSDK_BUILD_TESTS=ON`（默认 ON）后会生成若干测试可执行文件，可用以下方式一键运行：

### 方式一：脚本（推荐）

```bash
cd FalconMindSDK
./scripts/run_tests.sh          # 使用默认 build 目录
./scripts/run_tests.sh build    # 指定 build 目录
```

脚本会依次执行 core_tests、flow_executor、node_factory、detection_packet、yolo_prepost、tracker_tests、pipeline_link、flow_executor_e2e，并输出 Passed/Failed 汇总。

### 方式二：CTest

```bash
cd FalconMindSDK/build
ctest --output-on-failure
```

上述 8 个测试已在 CMake 中通过 `add_test` 注册，性能/压测可执行文件未纳入 CTest，需单独运行。
