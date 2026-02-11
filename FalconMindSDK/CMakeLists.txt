cmake_minimum_required(VERSION 3.16)
project(FalconMindSDK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FALCONMINDSDK_BUILD_TESTS "Build FalconMindSDK test/demo programs" ON)
option(FALCONMINDSDK_BUILD_PYTHON "Build FalconMindSDK Python bindings" ON)

# 检测是否为交叉编译
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(FALCONMINDSDK_CROSS_COMPILE_ARM64 TRUE)
    message(STATUS "检测到arm64交叉编译模式")
else()
    set(FALCONMINDSDK_CROSS_COMPILE_ARM64 FALSE)
endif()

if(FALCONMINDSDK_BUILD_TESTS)
    enable_testing()
endif()

# 推理后端：默认 OFF，无 GPU/NPU 时可正常编译；开启后需自行安装对应库并实现 load/run
option(FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND "Build with real ONNXRuntime inference backend (requires ONNXRuntime)" OFF)
option(FALCONMINDSDK_BUILD_RKNN_BACKEND "Build with real RKNN inference backend (requires RKNN-Toolkit2/board lib)" OFF)
option(FALCONMINDSDK_BUILD_TENSORRT_BACKEND "Build with real TensorRT inference backend (requires TensorRT+CUDA)" OFF)

# 设置第三方依赖库安装目录
set(FALCONMINDSDK_DEPEND_INSTALL_PREFIX "3rd/install/x86" CACHE STRING "依赖库安装目录 (3rd/install/x86 或 3rd/install/arm64)")
set(DEPEND_INSTALL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${FALCONMINDSDK_DEPEND_INSTALL_PREFIX})

# 交叉编译时跳过Python绑定（嵌入式目标通常不需要）
if(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    set(FALCONMINDSDK_BUILD_PYTHON OFF)
    message(STATUS "arm64交叉编译模式：禁用Python绑定")
    
    # 对于arm64交叉编译，使用本地3rd/install/arm64/include中的头文件
    set(ARM64_DEPEND_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/3rd/install/arm64/include")
    message(STATUS "arm64依赖头文件路径: ${ARM64_DEPEND_INCLUDE}")
endif()

# 查找nlohmann/json库（交叉编译时跳过系统查找，直接使用本地头文件）
if(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    # 交叉编译模式：使用本地3rd/install/arm64/include中的nlohmann/json
    set(nlohmann_json_FOUND TRUE)
    set(nlohmann_json_INCLUDE_DIR "${ARM64_DEPEND_INCLUDE}")
    message(STATUS "nlohmann/json: 使用本地arm64头文件 (${ARM64_DEPEND_INCLUDE}/nlohmann)")
else()
    # 本地编译模式：优先使用系统安装的版本
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        # 使用FetchContent下载到3rd目录
        include(FetchContent)
        FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(json)
        set(nlohmann_json_FOUND TRUE)
        message(STATUS "nlohmann/json: Built from source (cached in ${CMAKE_CURRENT_SOURCE_DIR}/3rd/json-src)")
    else()
        message(STATUS "nlohmann/json: Using system installation")
    endif()
endif()

# 查找cpp-httplib库（优先使用系统安装的版本）
find_package(httplib QUIET)
if(NOT httplib_FOUND)
    # 使用FetchContent下载到3rd目录（会自动缓存，不会重复下载）
    include(FetchContent)
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.14.3
        GIT_SHALLOW TRUE  # 只下载最新提交，不下载完整历史，加快下载速度
    )
    # FetchContent会自动检查缓存，如果3rd目录已存在则不会重新下载
    FetchContent_MakeAvailable(httplib)
    set(httplib_FOUND TRUE)
    message(STATUS "cpp-httplib: Using FetchContent (cached in ${CMAKE_CURRENT_SOURCE_DIR}/3rd/httplib-src)")
else()
    message(STATUS "cpp-httplib: Using system installation")
endif()

add_library(falconmind_sdk
    src/core/Pipeline.cpp
    src/core/Node.cpp
    src/core/Pad.cpp
    src/core/Caps.cpp
    src/core/Bus.cpp
    src/core/NodeFactory.cpp
    src/core/FlowExecutor.cpp
    src/c_api/falconmind_sdk_c_api.cpp
    src/flight/FlightConnectionService.cpp
    src/flight/FlightNodes.cpp
    src/sensors/CameraSourceNode.cpp
    src/sensors/LidarSourceNode.cpp
    src/sensors/ImuSourceNode.cpp
    src/sensors/GnssSourceNode.cpp
    src/perception/DummyDetectionNode.cpp
    src/perception/PerceptionPluginManager.cpp
    src/perception/OnnxRuntimeDetectorBackend.cpp
    src/perception/RknnDetectorBackend.cpp
    src/perception/TensorRtDetectorBackend.cpp
    src/perception/DetectorConfigLoader.cpp
    src/perception/DetectionResultPacket.cpp
    src/perception/YoloPrePostProcess.cpp
    src/perception/SimpleTrackerBackend.cpp
    src/perception/SortTrackerBackend.cpp
    src/perception/TrackingTransformNode.cpp
    src/perception/VisualSlamNode.cpp
    src/perception/LidarSlamNode.cpp
    src/perception/EnvironmentDetectionNode.cpp
    src/perception/LowLightAdaptationNode.cpp
    src/perception/SlamServiceClientFromFile.cpp
    src/cluster/ClusterStateSourceNode.cpp
    src/mission/BehaviorTree.cpp
    src/mission/SearchPathPlannerNode.cpp
    src/mission/EventReporterNode.cpp
    src/mission/SearchMissionAction.cpp
    src/telemetry/TelemetryPublisher.cpp
)

# 为Python绑定添加-fPIC编译选项（Position Independent Code）
if(FALCONMINDSDK_BUILD_PYTHON)
    set_target_properties(falconmind_sdk PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

target_include_directories(falconmind_sdk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接nlohmann/json库
if(nlohmann_json_FOUND)
    if(FALCONMINDSDK_CROSS_COMPILE_ARM64)
        # 交叉编译模式：直接添加头文件路径（nlohmann/json是头文件库）
        target_include_directories(falconmind_sdk PUBLIC "${nlohmann_json_INCLUDE_DIR}")
    else()
        # 本地编译模式：使用CMake包
        target_link_libraries(falconmind_sdk PUBLIC nlohmann_json::nlohmann_json)
    endif()
endif()

# 链接cpp-httplib库（仅用于FlowExecutor的HTTP功能）
if(httplib_FOUND)
    target_link_libraries(falconmind_sdk PRIVATE httplib::httplib)
    if(NOT FALCONMINDSDK_CROSS_COMPILE_ARM64)
        # 交叉编译模式跳过openssl（嵌入式设备通常不需要HTTPS）
        target_compile_definitions(falconmind_sdk PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    endif()
endif()

# 可选推理后端：ONNXRuntime — 选项 ON 时尝试查找并链接，未找到则 WARNING 且不定义宏（保持 stub）
if(FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND)
    set(ONNXRUNTIME_ROOT "$ENV{ONNXRUNTIME_ROOT}" CACHE PATH "ONNXRuntime install root (include/ and lib/)")
    if(ONNXRUNTIME_ROOT)
        set(ONNXRUNTIME_SEARCH_PATHS "${ONNXRUNTIME_ROOT}")
    else()
        set(ONNXRUNTIME_SEARCH_PATHS "/usr/local" "/opt/onnxruntime")
    endif()
    find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATH_SUFFIXES include
        PATHS ${ONNXRUNTIME_SEARCH_PATHS}
    )
    find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        PATH_SUFFIXES lib lib64
        PATHS ${ONNXRUNTIME_SEARCH_PATHS}
    )
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
        target_include_directories(falconmind_sdk PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
        target_link_libraries(falconmind_sdk PRIVATE "${ONNXRUNTIME_LIBRARY}")
        target_compile_definitions(falconmind_sdk PRIVATE FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED=1)
        message(STATUS "FalconMindSDK: ONNXRuntime backend enabled and linked: ${ONNXRUNTIME_LIBRARY}")
    else()
        message(WARNING "ONNXRuntime not found (set ONNXRUNTIME_ROOT or install to /usr/local). OnnxRuntimeDetectorBackend will remain stub.")
    endif()
endif()
# 主平台推理后端：RKNN（RK1126B/RK3576/RK3588）。选项 ON 时尝试查找并链接，未找到则 WARNING 且不定义宏（保持 stub）
if(FALCONMINDSDK_BUILD_RKNN_BACKEND)
    set(RKNN_SDK_ROOT "$ENV{RKNN_SDK_ROOT}" CACHE PATH "RKNN SDK root (include/ with rknn_api.h, lib/ or runtime/.../ with librknnrt.so)")
    if(RKNN_SDK_ROOT)
        set(RKNN_SEARCH_PATHS "${RKNN_SDK_ROOT}")
    else()
        set(RKNN_SEARCH_PATHS "/usr/local" "/opt/rknn")
    endif()
    find_path(RKNN_INCLUDE_DIR
        NAMES rknn_api.h
        PATH_SUFFIXES include
        PATHS ${RKNN_SEARCH_PATHS}
    )
    find_library(RKNN_LIBRARY
        NAMES rknnrt
        PATH_SUFFIXES lib lib64 aarch64 armhf runtime/Linux/librknn_api/aarch64 runtime/Linux/librknn_api/armhf
        PATHS ${RKNN_SEARCH_PATHS}
    )
    if(RKNN_INCLUDE_DIR AND RKNN_LIBRARY)
        target_include_directories(falconmind_sdk PRIVATE "${RKNN_INCLUDE_DIR}")
        target_link_libraries(falconmind_sdk PRIVATE "${RKNN_LIBRARY}")
        target_compile_definitions(falconmind_sdk PRIVATE FALCONMINDSDK_RKNN_BACKEND_ENABLED=1)
        message(STATUS "FalconMindSDK: RKNN backend enabled and linked: ${RKNN_LIBRARY}")
    else()
        message(WARNING "RKNN not found (set RKNN_SDK_ROOT to SDK root with include/rknn_api.h and librknnrt.so). RknnDetectorBackend will remain stub.")
    endif()
endif()
if(FALCONMINDSDK_BUILD_TENSORRT_BACKEND)
    # find_package(TensorRT REQUIRED) 等
    target_compile_definitions(falconmind_sdk PRIVATE FALCONMINDSDK_TENSORRT_BACKEND_ENABLED=1)
    message(STATUS "FalconMindSDK: TensorRT backend enabled (ensure TensorRT is linked)")
endif()

if(FALCONMINDSDK_BUILD_TESTS)
    add_executable(falconmind_sdk_demo
        demo/pipeline_test_main.cpp
        demo/TestNodes.cpp
    )
    target_link_libraries(falconmind_sdk_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_flight_demo
        demo/flight_demo_main.cpp
    )
    target_link_libraries(falconmind_flight_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_camera_detection_demo
        demo/camera_detection_demo_main.cpp
        demo/TestNodes.cpp
    )
    target_link_libraries(falconmind_camera_detection_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_detector_config_demo
        demo/detector_config_demo_main.cpp
    )
    target_link_libraries(falconmind_detector_config_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_tracking_demo
        demo/tracking_demo_main.cpp
        demo/TestNodes.cpp
    )
    target_link_libraries(falconmind_tracking_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_behavior_tree_flight_demo
        demo/behavior_tree_flight_demo_main.cpp
    )
    target_link_libraries(falconmind_behavior_tree_flight_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_telemetry_demo
        demo/telemetry_demo_main.cpp
    )
    target_link_libraries(falconmind_telemetry_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_search_mission_demo
        demo/search_mission_demo_main.cpp
    )
    target_link_libraries(falconmind_search_mission_demo PRIVATE falconmind_sdk)

    add_executable(falconmind_sdk_core_tests
        tests/core_pipeline_tests.cpp
    )
    target_link_libraries(falconmind_sdk_core_tests PRIVATE falconmind_sdk)
    
    add_executable(falconmind_flow_executor_tests
        tests/test_flow_executor.cpp
    )
    target_link_libraries(falconmind_flow_executor_tests PRIVATE falconmind_sdk)
    
    add_executable(falconmind_node_factory_tests
        tests/test_node_factory.cpp
    )
    target_link_libraries(falconmind_node_factory_tests PRIVATE falconmind_sdk)

    add_executable(falconmind_detection_packet_tests
        tests/test_detection_result_packet.cpp
    )
    target_link_libraries(falconmind_detection_packet_tests PRIVATE falconmind_sdk)

    add_executable(falconmind_yolo_prepost_tests
        tests/test_yolo_pre_post_process.cpp
    )
    target_link_libraries(falconmind_yolo_prepost_tests PRIVATE falconmind_sdk)

    add_executable(falconmind_tracker_tests
        tests/test_simple_tracker_backend.cpp
    )
    target_link_libraries(falconmind_tracker_tests PRIVATE falconmind_sdk)
    
    add_executable(falconmind_pipeline_link_tests
        tests/test_pipeline_link.cpp
    )
    target_link_libraries(falconmind_pipeline_link_tests PRIVATE falconmind_sdk)
    
    add_executable(falconmind_flow_executor_e2e_tests
        tests/test_flow_executor_e2e.cpp
    )
    target_link_libraries(falconmind_flow_executor_e2e_tests PRIVATE falconmind_sdk)
    
    add_executable(falconmind_flow_executor_performance_tests
        tests/test_flow_executor_performance.cpp
    )
    target_link_libraries(falconmind_flow_executor_performance_tests PRIVATE falconmind_sdk)
    
    # Performance benchmark executable (Stage 1)
    add_executable(test_performance_benchmark
        tests/test_performance_benchmark.cpp
    )
    target_link_libraries(test_performance_benchmark PRIVATE falconmind_sdk)

    # CTest: register unit/integration tests (excludes long-running performance/benchmark)
    add_test(NAME falconmind_sdk_core_tests COMMAND falconmind_sdk_core_tests)
    add_test(NAME falconmind_flow_executor_tests COMMAND falconmind_flow_executor_tests)
    add_test(NAME falconmind_node_factory_tests COMMAND falconmind_node_factory_tests)
    add_test(NAME falconmind_detection_packet_tests COMMAND falconmind_detection_packet_tests)
    add_test(NAME falconmind_yolo_prepost_tests COMMAND falconmind_yolo_prepost_tests)
    add_test(NAME falconmind_tracker_tests COMMAND falconmind_tracker_tests)
    add_test(NAME falconmind_pipeline_link_tests COMMAND falconmind_pipeline_link_tests)
    add_test(NAME falconmind_flow_executor_e2e_tests COMMAND falconmind_flow_executor_e2e_tests)
endif()

# Python bindings using pybind11
if(FALCONMINDSDK_BUILD_PYTHON AND NOT FALCONMINDSDK_CROSS_COMPILE_ARM64)
    find_package(pybind11 QUIET)
    if(NOT pybind11_FOUND)
        # Download pybind11 if not found
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.11.1
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(pybind11)
        message(STATUS "pybind11: Using FetchContent (cached in ${CMAKE_CURRENT_SOURCE_DIR}/3rd/pybind11-src)")
    else()
        message(STATUS "pybind11: Using system installation")
    endif()

    # Find Python
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    
    # Python module
    pybind11_add_module(falconmind_sdk_py
        python/falconmind_sdk.cpp
    )
    
    target_link_libraries(falconmind_sdk_py PRIVATE falconmind_sdk)
    
    # Set output directory for Python module
    set_target_properties(falconmind_sdk_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python
        OUTPUT_NAME falconmind_sdk
    )
    
    message(STATUS "Python bindings will be installed to: ${CMAKE_CURRENT_SOURCE_DIR}/python")
elseif(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    message(STATUS "Python bindings: 跳过 (arm64交叉编译模式)")
endif()

# =============================================================================
# 安装配置 - 支持x86和arm64两个平台
# =============================================================================
# 使用方法:
#   x86: cmake .. -DFALCONMINDSDK_INSTALL_PREFIX=install/x86 && make install
#   arm64: cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake -DFALCONMINDSDK_INSTALL_PREFIX=install/arm64 && make install
# =============================================================================

set(FALCONMINDSDK_INSTALL_PREFIX "install/x86" CACHE STRING "安装目录前缀 (install/x86 或 install/arm64)")

# 安装根目录
set(INSTALL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${FALCONMINDSDK_INSTALL_PREFIX})

# 库文件安装目录
set(INSTALL_LIB_DIR ${INSTALL_ROOT}/lib)
# 头文件安装目录
set(INSTALL_INCLUDE_DIR ${INSTALL_ROOT}/include)

# 安装库文件
install(TARGETS falconmind_sdk
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
)

# 安装头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${INSTALL_INCLUDE_DIR}
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "")
message(STATUS "=== FalconMindSDK 安装配置 ===")
message(STATUS "  安装前缀: ${FALCONMINDSDK_INSTALL_PREFIX}")
message(STATUS "  库文件目录: ${INSTALL_LIB_DIR}")
message(STATUS "  头文件目录: ${INSTALL_INCLUDE_DIR}")
message(STATUS "  执行安装: make install")
message(STATUS "")

# =============================================================================
# NodeAgent集成 (作为SDK子目录统一编译)
# =============================================================================
option(FALCONMINDSDK_BUILD_NODEAGENT "Build NodeAgent (unified with SDK)" ON)

if(FALCONMINDSDK_BUILD_NODEAGENT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent/CMakeLists.txt")
        message(STATUS "NodeAgent: 集成编译")
        add_subdirectory(NodeAgent)
    else()
        message(WARNING "NodeAgent目录不存在: ${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent")
    endif()
endif()