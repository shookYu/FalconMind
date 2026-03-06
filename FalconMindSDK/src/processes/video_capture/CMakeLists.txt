# Video Capture Process - CMake Build Configuration
# Production GStreamer RTSP pipeline with DDS integration

cmake_minimum_required(VERSION 3.16)
project(VideoCaptureProcess VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Options
# =============================================================================

option(FALCONMIND_PLATFORM "Target platform (x86_64, RK3588, RK3576)" "x86_64")
option(ENABLE_HW_ACCEL "Enable hardware acceleration" ON)
option(ENABLE_RKMPP "Enable Rockchip MPP encoder" OFF)

# Auto-detect RK3588
if(FALCONMIND_PLATFORM STREQUAL "RK3588" OR FALCONMIND_PLATFORM STREQUAL "RK3576")
    set(ENABLE_RKMPP ON)
    message(STATUS "Enabling Rockchip MPP hardware encoder for ${FALCONMIND_PLATFORM}")
endif()

# =============================================================================
# Find Dependencies
# =============================================================================

# GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_RTSP REQUIRED gstreamer-rtsp-server-1.0)

# Fast DDS
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)

# YAML-CPP
find_package(yaml-cpp REQUIRED)

# Threads
find_package(Threads REQUIRED)

# =============================================================================
# Include Directories
# =============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../fastdds/generated
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_RTSP_INCLUDE_DIRS}
)

# =============================================================================
# Source Files
# =============================================================================

set(SOURCES
    video_capture_process.cpp
)

# =============================================================================
# Executable
# =============================================================================

add_executable(video_capture_process ${SOURCES})

# Link libraries
target_link_libraries(video_capture_process
    # GStreamer
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_RTSP_LIBRARIES}
    
    # Fast DDS
    fastcdr
    fastrtps
    
    # DDS Types (from parent project)
    falconmind_dds_types
    falconmind_dds
    
    # YAML
    yaml-cpp
    
    # System
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Compile options
target_compile_options(video_capture_process PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
    -Wall
    -Wextra
    -O3
    -march=native
)

# Platform-specific options
if(FALCONMIND_PLATFORM STREQUAL "RK3588")
    target_compile_definitions(video_capture_process PRIVATE
        PLATFORM_RK3588
        ENABLE_RKMPP
    )
    target_link_libraries(video_capture_process
        rockchip_mpp
        rockchip_vpu
    )
endif()

# =============================================================================
# Installation
# =============================================================================

install(TARGETS video_capture_process
    RUNTIME DESTINATION bin
)

install(FILES video_capture.yaml
    DESTINATION etc/falconmind
)

# =============================================================================
# Testing
# =============================================================================

if(BUILD_TESTING)
    add_executable(test_video_pipeline
        tests/test_pipeline.cpp
    )
    
    target_link_libraries(test_video_pipeline
        ${GSTREAMER_LIBRARIES}
        gtest
    )
    
    add_test(NAME VideoPipelineTest COMMAND test_video_pipeline)
endif()
