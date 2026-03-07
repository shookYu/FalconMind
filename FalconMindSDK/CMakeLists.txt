cmake_minimum_required(VERSION 3.16)
project(FalconMindSDK LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(FALCONMINDSDK_BUILD_TESTS "Build FalconMindSDK test/demo programs" ON)
option(FALCONMINDSDK_BUILD_PYTHON "Build FalconMindSDK Python bindings" ON)
option(FALCONMINDSDK_BUILD_NODEAGENT "Build NodeAgent (unified with SDK)" ON)
option(FALCONMINDSDK_BUILD_INTERFACE_ADAPTER "Build SDK interface adapter for NodeAgent" ON)

# Inference backends
option(FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND "Build with ONNXRuntime inference backend" OFF)
option(FALCONMINDSDK_BUILD_RKNN_BACKEND "Build with RKNN inference backend (RK3588/RK3576/RV1126B)" ON)
option(FALCONMINDSDK_BUILD_TENSORRT_BACKEND "Build with TensorRT inference backend (requires CUDA)" OFF)
option(FALCONMINDSDK_BUILD_ROS2 "Build with ROS2 integration (requires ROS2 Humble/Iron/Jazzy)" OFF)

# Offline build mode - skip network downloads
option(FALCONMINDSDK_OFFLINE_BUILD "Enable offline build mode (use local dependencies only)" OFF)

# Cross-compilation detection
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(FALCONMINDSDK_CROSS_COMPILE_ARM64 TRUE)
    message(STATUS "Detected ARM64 cross-compilation mode")
else()
    set(FALCONMINDSDK_CROSS_COMPILE_ARM64 FALSE)
endif()

if(FALCONMINDSDK_BUILD_TESTS)
    enable_testing()
    
    # Coverage options
    option(FALCONMINDSDK_ENABLE_COVERAGE "Enable test coverage reporting" OFF)
    if(FALCONMINDSDK_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

# Set third-party dependency install directory
set(FALCONMINDSDK_DEPEND_INSTALL_PREFIX "3rd/install/x86" CACHE STRING "Dependency install directory (3rd/install/x86 or 3rd/install/arm64)")
set(DEPEND_INSTALL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${FALCONMINDSDK_DEPEND_INSTALL_PREFIX})

# Skip Python bindings for cross-compilation
if(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    set(FALCONMINDSDK_BUILD_PYTHON OFF)
    message(STATUS "ARM64 cross-compilation: Python bindings disabled")
    
    # Use local ARM64 headers
    set(ARM64_DEPEND_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/3rd/install/arm64/include")
    message(STATUS "ARM64 dependency headers: ${ARM64_DEPEND_INCLUDE}")
endif()

# ============================================================================
# Dependencies - nlohmann/json
# ============================================================================
if(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    # Cross-compilation: use local headers
    set(nlohmann_json_FOUND TRUE)
    set(nlohmann_json_INCLUDE_DIR "${ARM64_DEPEND_INCLUDE}")
    message(STATUS "nlohmann/json: Using local ARM64 headers")
elseif(FALCONMINDSDK_OFFLINE_BUILD)
    # Offline build: check local directories only
    find_package(nlohmann_json QUIET PATHS ${DEPEND_INSTALL_ROOT} NO_DEFAULT_PATH)
    if(NOT nlohmann_json_FOUND)
        # Check bundled headers
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/json/single_include/nlohmann/json.hpp")
            set(nlohmann_json_FOUND TRUE)
            set(nlohmann_json_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rd/json/single_include")
            message(STATUS "nlohmann/json: Using bundled headers")
        else()
            message(FATAL_ERROR "Offline build enabled but nlohmann/json not found. Please install to ${DEPEND_INSTALL_ROOT} or add to 3rd/json/")
        endif()
    endif()
else()
    # Online build: try system first, then FetchContent
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(json)
        set(nlohmann_json_FOUND TRUE)
        message(STATUS "nlohmann/json: Built from source")
    else()
        message(STATUS "nlohmann/json: Using system installation")
    endif()
endif()

# ============================================================================
# Dependencies - cpp-httplib
# ============================================================================
if(FALCONMINDSDK_OFFLINE_BUILD)
    find_package(httplib QUIET PATHS ${DEPEND_INSTALL_ROOT} NO_DEFAULT_PATH)
    if(NOT httplib_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/cpp-httplib/httplib.h")
            add_library(httplib::httplib INTERFACE IMPORTED)
            set_target_properties(httplib::httplib PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rd/cpp-httplib"
            )
            set(httplib_FOUND TRUE)
            message(STATUS "cpp-httplib: Using bundled headers")
        else()
            message(FATAL_ERROR "Offline build enabled but cpp-httplib not found.")
        endif()
    endif()
else()
    find_package(httplib QUIET)
    if(NOT httplib_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            httplib
            GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
            GIT_TAG v0.14.3
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(httplib)
        set(httplib_FOUND TRUE)
        message(STATUS "cpp-httplib: Built from source")
    else()
        message(STATUS "cpp-httplib: Using system installation")
    endif()
endif()

# ============================================================================
# Main library
# ============================================================================
add_library(falconmind_sdk
    src/core/Pipeline.cpp
    src/core/Node.cpp
    src/core/Pad.cpp
    src/core/Caps.cpp
    src/core/Bus.cpp
    src/core/NodeFactory.cpp
    src/core/FlowExecutor.cpp
    src/flow/flow_node.cpp
    src/flow/flow_executor_v2.cpp
    src/flow/nodes/vins_nodes.cpp
    src/flow/nodes/gps_defense_nodes.cpp
    src/flow/nodes/perception_nodes.cpp
    src/flow/nodes/control_nodes.cpp
    src/flow/nodes/navigation_nodes.cpp
    src/c_api/falconmind_sdk_c_api.cpp
    src/flight/FlightConnectionService.cpp
    src/flight/FlightNodes.cpp
    src/flight/GeofenceMonitorNode.cpp
    src/sensors/CameraSourceNode.cpp
    src/sensors/LidarSourceNode.cpp
    src/sensors/ImuSourceNode.cpp
    src/sensors/GnssSourceNode.cpp
    src/perception/DummyDetectionNode.cpp
    src/perception/TrackingTransformNode.cpp
    src/perception/PerceptionPluginManager.cpp
    src/perception/RknnDetectorBackend.cpp
    src/perception/EnvironmentDetectionNode.cpp
    # src/perception/OnnxRuntimeDetectorBackend.cpp  # FIXME: x86/x64 only, not for ARM64/RK3576
    # src/perception/TensorRtDetectorBackend.cpp  # FIXME: requires CUDA
    src/perception/LowLightAdaptationNode.cpp
    src/perception/LidarSlamNode.cpp
    src/perception/DetectorConfigLoader.cpp
    src/perception/DetectionResultPacket.cpp
    src/perception/YoloPrePostProcess.cpp
    src/perception/SimpleTrackerBackend.cpp
    src/perception/SortTrackerBackend.cpp
    # src/perception/VisualSlamNode.cpp  # FIXME: requires VINS-Fusion
    src/perception/SlamServiceClientFromFile.cpp
    src/cluster/ClusterStateSourceNode.cpp
    src/mission/BehaviorTree.cpp
    src/mission/SearchPathPlannerNode.cpp
    src/mission/EventReporterNode.cpp
    src/mission/SearchMissionAction.cpp
    src/mission/GeofenceMonitorNode.cpp
    src/telemetry/TelemetryPublisher.cpp
    src/high_level/PerceptionPipeline.cpp
    src/high_level/MissionPipeline.cpp
    src/high_level/FlightPipeline.cpp
    src/high_level/MavlinkClient.cpp

    # Plugin system
    src/plugin/PluginManager.cpp
    src/plugin/CapabilityRegistry.cpp
    src/plugin/BuiltinPlugins.cpp
)

# PIC for Python bindings
if(FALCONMINDSDK_BUILD_PYTHON)
    set_target_properties(falconmind_sdk PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# ROS2 integration
if(FALCONMINDSDK_BUILD_ROS2)
    find_package(rclcpp QUIET)
    find_package(geometry_msgs QUIET)
    find_package(sensor_msgs QUIET)
    find_package(nav_msgs QUIET)
    
    if(rclcpp_FOUND AND geometry_msgs_FOUND AND sensor_msgs_FOUND AND nav_msgs_FOUND)
        message(STATUS "ROS2 found - enabling ROS2 bridge")
        target_compile_definitions(falconmind_sdk PUBLIC FALCONMINDSDK_WITH_ROS2)
        target_sources(falconmind_sdk PRIVATE src/ros2/FalconMindROS2Node.cpp)
        target_link_libraries(falconmind_sdk PUBLIC
            rclcpp::rclcpp
            geometry_msgs::geometry_msgs__rosidl_typesupport_cpp
            sensor_msgs::sensor_msgs__rosidl_typesupport_cpp
            nav_msgs::nav_msgs__rosidl_typesupport_cpp
        )
    else()
        message(WARNING "ROS2 packages not found - ROS2 bridge disabled")
        set(FALCONMINDSDK_BUILD_ROS2 OFF)
    endif()
endif()

target_include_directories(falconmind_sdk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link nlohmann/json
if(nlohmann_json_FOUND)
    if(FALCONMINDSDK_CROSS_COMPILE_ARM64 OR FALCONMINDSDK_OFFLINE_BUILD)
        target_include_directories(falconmind_sdk PUBLIC "${nlohmann_json_INCLUDE_DIR}")
    else()
        target_link_libraries(falconmind_sdk PUBLIC nlohmann_json::nlohmann_json)
    endif()
endif()

# Link cpp-httplib
if(httplib_FOUND)
    if(FALCONMINDSDK_OFFLINE_BUILD AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/cpp-httplib/httplib.h")
        target_include_directories(falconmind_sdk PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rd/cpp-httplib")
    else()
        target_link_libraries(falconmind_sdk PRIVATE httplib::httplib)
    endif()
    if(NOT FALCONMINDSDK_CROSS_COMPILE_ARM64)
        target_compile_definitions(falconmind_sdk PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    endif()
endif()

# RKNN backend
if(FALCONMINDSDK_BUILD_RKNN_BACKEND)
    set(RKNN_SDK_ROOT "$ENV{RKNN_SDK_ROOT}" CACHE PATH "RKNN SDK root")
    if(RKNN_SDK_ROOT)
        set(RKNN_SEARCH_PATHS "${RKNN_SDK_ROOT}")
    else()
        set(RKNN_SEARCH_PATHS "/usr/local" "/opt/rknn" "${DEPEND_INSTALL_ROOT}")
    endif()
    find_path(RKNN_INCLUDE_DIR
        NAMES rknn_api.h
        PATH_SUFFIXES include
        PATHS ${RKNN_SEARCH_PATHS}
        NO_DEFAULT_PATH
    )
    find_library(RKNN_LIBRARY
        NAMES rknnrt
        PATH_SUFFIXES lib lib64 aarch64
        PATHS ${RKNN_SEARCH_PATHS}
        NO_DEFAULT_PATH
    )
    if(RKNN_INCLUDE_DIR AND RKNN_LIBRARY)
        target_include_directories(falconmind_sdk PRIVATE "${RKNN_INCLUDE_DIR}")
        target_link_libraries(falconmind_sdk PRIVATE "${RKNN_LIBRARY}")
        target_compile_definitions(falconmind_sdk PRIVATE FALCONMINDSDK_RKNN_BACKEND_ENABLED=1)
        message(STATUS "RKNN backend enabled: ${RKNN_LIBRARY}")
    else()
        message(WARNING "RKNN not found. Set RKNN_SDK_ROOT or install to ${DEPEND_INSTALL_ROOT}")
    endif()
endif()

# ============================================================================
# Test executables
# ============================================================================
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
    target_link_libraries(falconmind_flow_executor_tests PRIVATE falconmind_sdk)
    
    # Flow Node Base Tests (百日攻坚新增)
    add_executable(falconmind_flow_node_base_tests
        tests/flow/test_flow_node_base.cpp
    )
    target_link_libraries(falconmind_flow_node_base_tests PRIVATE falconmind_sdk)
    add_test(NAME FlowNodeBaseTests COMMAND falconmind_flow_node_base_tests)
    
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
    
    add_executable(test_performance_benchmark
        tests/test_performance_benchmark.cpp
    )
    target_link_libraries(test_performance_benchmark PRIVATE falconmind_sdk)

    # CTest registration
    add_test(NAME falconmind_sdk_core_tests COMMAND falconmind_sdk_core_tests)
    add_test(NAME falconmind_flow_executor_tests COMMAND falconmind_flow_executor_tests)
    add_test(NAME falconmind_node_factory_tests COMMAND falconmind_node_factory_tests)
    add_test(NAME falconmind_detection_packet_tests COMMAND falconmind_detection_packet_tests)
    add_test(NAME falconmind_yolo_prepost_tests COMMAND falconmind_yolo_prepost_tests)
    add_test(NAME falconmind_tracker_tests COMMAND falconmind_tracker_tests)
    add_test(NAME falconmind_pipeline_link_tests COMMAND falconmind_pipeline_link_tests)
    add_test(NAME falconmind_flow_executor_e2e_tests COMMAND falconmind_flow_executor_e2e_tests)
endif()

# ============================================================================
# Python bindings
# ============================================================================
if(FALCONMINDSDK_BUILD_PYTHON AND NOT FALCONMINDSDK_CROSS_COMPILE_ARM64)
    if(FALCONMINDSDK_OFFLINE_BUILD)
        find_package(pybind11 QUIET PATHS ${DEPEND_INSTALL_ROOT} NO_DEFAULT_PATH)
        if(NOT pybind11_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/pybind11/include")
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd/pybind11)
            set(pybind11_FOUND TRUE)
            message(STATUS "pybind11: Using bundled")
        endif()
    else()
        find_package(pybind11 QUIET)
        if(NOT pybind11_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                pybind11
                GIT_REPOSITORY https://github.com/pybind/pybind11.git
                GIT_TAG v2.11.1
                GIT_SHALLOW TRUE
            )
            FetchContent_MakeAvailable(pybind11)
            message(STATUS "pybind11: Using FetchContent")
        else()
            message(STATUS "pybind11: Using system installation")
        endif()
    endif()
    
    if(pybind11_FOUND)
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        
        # Main SDK Python module
        pybind11_add_module(falconmind_sdk_py
            python/falconmind_sdk.cpp
        )
        
        target_link_libraries(falconmind_sdk_py PRIVATE falconmind_sdk)
        
        set_target_properties(falconmind_sdk_py PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python
            OUTPUT_NAME falconmind_sdk
        )
        
        # Denied environment nodes Python module
        pybind11_add_module(denied_env_nodes_py
            python/denied_env_nodes.cpp
        )
        
        target_link_libraries(denied_env_nodes_py PRIVATE falconmind_sdk)
        
        set_target_properties(denied_env_nodes_py PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python
            OUTPUT_NAME denied_env_nodes
        )
        
        message(STATUS "Python bindings will be installed to: ${CMAKE_CURRENT_SOURCE_DIR}/python")
        message(STATUS "  - falconmind_sdk (Core SDK API)")
        message(STATUS "  - denied_env_nodes (Denied Environment Nodes)")
    endif()
elseif(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    message(STATUS "Python bindings: Skipped (ARM64 cross-compilation)")
endif()
# Python bindings
# ============================================================================
if(FALCONMINDSDK_BUILD_PYTHON AND NOT FALCONMINDSDK_CROSS_COMPILE_ARM64)
    if(FALCONMINDSDK_OFFLINE_BUILD)
        find_package(pybind11 QUIET PATHS ${DEPEND_INSTALL_ROOT} NO_DEFAULT_PATH)
        if(NOT pybind11_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/pybind11/include")
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd/pybind11)
            set(pybind11_FOUND TRUE)
            message(STATUS "pybind11: Using bundled")
        endif()
    else()
        find_package(pybind11 QUIET)
        if(NOT pybind11_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                pybind11
                GIT_REPOSITORY https://github.com/pybind/pybind11.git
                GIT_TAG v2.11.1
                GIT_SHALLOW TRUE
            )
            FetchContent_MakeAvailable(pybind11)
            message(STATUS "pybind11: Using FetchContent")
        else()
            message(STATUS "pybind11: Using system installation")
        endif()
    endif()
    
    if(pybind11_FOUND)
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        
        pybind11_add_module(falconmind_sdk_py
            python/falconmind_sdk.cpp
        )
        
        target_link_libraries(falconmind_sdk_py PRIVATE falconmind_sdk)
        
        set_target_properties(falconmind_sdk_py PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python
            OUTPUT_NAME falconmind_sdk
        )
        
        message(STATUS "Python bindings will be installed to: ${CMAKE_CURRENT_SOURCE_DIR}/python")
    endif()
elseif(FALCONMINDSDK_CROSS_COMPILE_ARM64)
    message(STATUS "Python bindings: Skipped (ARM64 cross-compilation)")
endif()

# ============================================================================
# Installation
# ============================================================================
set(FALCONMINDSDK_INSTALL_PREFIX "install/x86" CACHE STRING "Installation prefix (install/x86 or install/arm64)")
set(INSTALL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${FALCONMINDSDK_INSTALL_PREFIX})
set(INSTALL_LIB_DIR ${INSTALL_ROOT}/lib)
set(INSTALL_INCLUDE_DIR ${INSTALL_ROOT}/include)

install(TARGETS falconmind_sdk
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${INSTALL_INCLUDE_DIR}
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "")
message(STATUS "=== FalconMindSDK Installation ===")
message(STATUS "  Install prefix: ${FALCONMINDSDK_INSTALL_PREFIX}")
message(STATUS "  Library directory: ${INSTALL_LIB_DIR}")
message(STATUS "  Include directory: ${INSTALL_INCLUDE_DIR}")
message(STATUS "  Run: make install")
message(STATUS "")

# ============================================================================
# NodeAgent integration
# ============================================================================
if(FALCONMINDSDK_BUILD_NODEAGENT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent/CMakeLists.txt")
        message(STATUS "NodeAgent: Integrated build")
        add_subdirectory(NodeAgent)
    else()
        message(WARNING "NodeAgent directory not found: ${CMAKE_CURRENT_SOURCE_DIR}/NodeAgent")
    endif()
endif()

# ============================================================================
# SDK Interface Adapter (for NodeAgent decoupling)
# ============================================================================
if(FALCONMINDSDK_BUILD_INTERFACE_ADAPTER)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/adapter/CMakeLists.txt")
        include(${CMAKE_CURRENT_SOURCE_DIR}/src/adapter/CMakeLists.txt)
    else()
        message(WARNING "SDK adapter not found: ${CMAKE_CURRENT_SOURCE_DIR}/src/adapter/")
    endif()
endif()

# ============================================================================
# Build summary
# ============================================================================
message(STATUS "")
message(STATUS "=== FalconMindSDK Build Configuration ===")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Cross-compile ARM64: ${FALCONMINDSDK_CROSS_COMPILE_ARM64}")
message(STATUS "  Offline build: ${FALCONMINDSDK_OFFLINE_BUILD}")
message(STATUS "  Tests: ${FALCONMINDSDK_BUILD_TESTS}")
message(STATUS "  Python bindings: ${FALCONMINDSDK_BUILD_PYTHON}")
message(STATUS "  NodeAgent: ${FALCONMINDSDK_BUILD_NODEAGENT}")
message(STATUS "  Interface adapter: ${FALCONMINDSDK_BUILD_INTERFACE_ADAPTER}")
message(STATUS "  RKNN backend: ${FALCONMINDSDK_BUILD_RKNN_BACKEND}")
message(STATUS "  ROS2: ${FALCONMINDSDK_BUILD_ROS2}")
message(STATUS "")
