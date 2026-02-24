cmake_minimum_required(VERSION 3.16)
project(07_rknn_backend_rk3588 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FalconMind SDK path
set(FALCONMIND_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

# Find RKNN Toolkit2
# RKNN Toolkit2 provides runtime API for both PC simulation and board deployment
find_path(RKNN_API_INCLUDE_DIR 
    NAMES rknn_api.h
    PATHS 
        /usr/include
        /usr/local/include
        /opt/rknn-toolkit2/include
        $ENV{RKNN_TOOLKIT2_PATH}/include
)

find_library(RKNN_API_LIBRARY 
    NAMES rknn_api librknn_api
    PATHS 
        /usr/lib
        /usr/local/lib
        /opt/rknn-toolkit2/lib
        $ENV{RKNN_TOOLKIT2_PATH}/lib
)

# Check if RKNN Toolkit2 is found
if(NOT RKNN_API_INCLUDE_DIR OR NOT RKNN_API_LIBRARY)
    message(FATAL_ERROR "
RKNN Toolkit2 not found!

RKNN Toolkit2 is required for building this example.
Please install RKNN Toolkit2:

1. Download from: https://github.com/rockchip-linux/rknn-toolkit2
2. Install: pip install rknn-toolkit2
3. Set environment variable (optional):
   export RKNN_TOOLKIT2_PATH=/path/to/rknn-toolkit2

For detailed installation instructions, see:
https://github.com/rockchip-linux/rknn-toolkit2/blob/master/doc/01_RKNN_Toolkit2_Installation_Guide.md
    ")
endif()

message(STATUS "RKNN Toolkit2 found:")
message(STATUS "  Include: ${RKNN_API_INCLUDE_DIR}")
message(STATUS "  Library: ${RKNN_API_LIBRARY}")

# Add executable
add_executable(07_rknn_backend_rk3588 src/main.cpp)

# Include directories
target_include_directories(07_rknn_backend_rk3588 PRIVATE 
    ${FALCONMIND_SDK}/include
    ${RKNN_API_INCLUDE_DIR}
)

# Find FalconMind SDK library
find_library(FALCONMIND_SDK_LIB 
    NAMES falconmind_sdk 
    PATHS 
        ${FALCONMIND_SDK}/build_arm64 
        ${FALCONMIND_SDK}/install/arm64/lib
        ${FALCONMIND_SDK}/build
        ${FALCONMIND_SDK}/install/x86/lib
    NO_DEFAULT_PATH
)

if(NOT FALCONMIND_SDK_LIB)
    message(FATAL_ERROR "FalconMind SDK library not found!")
endif()

# Link libraries
target_link_libraries(07_rknn_backend_rk3588 PRIVATE 
    ${FALCONMIND_SDK_LIB}
    ${RKNN_API_LIBRARY}
    pthread
    dl
)

# Enable RKNN backend
target_compile_definitions(07_rknn_backend_rk3588 PRIVATE 
    FALCONMINDSDK_RKNN_BACKEND_ENABLED=1
)

# Installation
install(TARGETS 07_rknn_backend_rk3588
    RUNTIME DESTINATION bin
)
