cmake_minimum_required(VERSION 3.16)
project(denied_env_tracking VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(USE_NPU "Use NPU acceleration" ON)
option(USE_MAVLINK "Use MAVLink communication" ON)

# 查找依赖
find_package(FalconMindSDK REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

if(USE_MAVLINK)
    find_package(MAVLink REQUIRED)
endif()

if(USE_NPU)
    find_package(RKNPU2 REQUIRED)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FalconMindSDK_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

if(USE_MAVLINK)
    include_directories(${MAVLINK_INCLUDE_DIRS})
endif()

# 源文件
set(NAVIGATION_SOURCES
    src/navigation/vins_initializer.cpp
    src/navigation/gps_defender.cpp
    src/navigation/position_fusion.cpp
)

set(PERCEPTION_SOURCES
    src/perception/yolo_detector.cpp
    src/perception/deepsort_tracker.cpp
    src/perception/distance_estimator.cpp
)

set(CONTROL_SOURCES
    src/control/ibvs_controller.cpp
    src/control/mavlink_interface.cpp
    src/control/gcs_interface.cpp
)

set(MISSION_SOURCES
    src/mission/mission_controller.cpp
)

set(SOURCES
    ${NAVIGATION_SOURCES}
    ${PERCEPTION_SOURCES}
    ${CONTROL_SOURCES}
    ${MISSION_SOURCES}
)

# 创建静态库
add_library(denied_env_tracking_lib STATIC ${SOURCES})

target_link_libraries(denied_env_tracking_lib
    FalconMindSDK::Core
    FalconMindSDK::Perception
    ${OpenCV_LIBS}
    Eigen3::Eigen
    pthread
)

if(USE_MAVLINK)
    target_link_libraries(denied_env_tracking_lib ${MAVLINK_LIBRARIES})
    target_compile_definitions(denied_env_tracking_lib PRIVATE USE_MAVLINK)
endif()

if(USE_NPU)
    target_link_libraries(denied_env_tracking_lib ${RKNPU2_LIBRARIES})
    target_compile_definitions(denied_env_tracking_lib PRIVATE USE_NPU)
endif()

# 主可执行文件
add_executable(denied_env_tracking src/main.cpp)

target_link_libraries(denied_env_tracking
    denied_env_tracking_lib
    FalconMindSDK::Core
    FalconMindSDK::Flight
)

# 安装
install(TARGETS denied_env_tracking
    RUNTIME DESTINATION bin
)

install(TARGETS denied_env_tracking_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

# 测试
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(test_vins tests/test_vins.cpp)
    target_link_libraries(test_vins denied_env_tracking_lib GTest::GTest)
    add_test(NAME VINSTest COMMAND test_vins)
    
    add_executable(test_tracking tests/test_tracking.cpp)
    target_link_libraries(test_tracking denied_env_tracking_lib GTest::GTest)
    add_test(NAME TrackingTest COMMAND test_tracking)
    
    add_executable(test_gps_defender tests/test_gps_defender.cpp)
    target_link_libraries(test_gps_defender denied_env_tracking_lib GTest::GTest)
    add_test(NAME GPSDefenderTest COMMAND test_gps_defender)
endif()

# 示例配置
if(BUILD_EXAMPLES)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config/example.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/config/example.yaml
        COPYONLY
    )
endif()

# 打印配置信息
message(STATUS "")
message(STATUS "Denied Environment Tracking Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  NPU acceleration: ${USE_NPU}")
message(STATUS "  MAVLink: ${USE_MAVLINK}")
message(STATUS "")
