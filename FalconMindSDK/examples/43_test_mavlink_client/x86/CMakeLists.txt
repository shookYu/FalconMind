cmake_minimum_required(VERSION 3.16)
project(test_mavlink_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FalconMind SDK
find_library(FALCONMIND_SDK 
    NAMES falconmind_sdk
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../build
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../install/x86/lib
    NO_DEFAULT_PATH
)

if(NOT FALCONMIND_SDK)
    message(FATAL_ERROR "FalconMind SDK not found. Please build the SDK first.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

# Executable
add_executable(test_mavlink_client_x86
    src/main.cpp
)

# Link with FalconMind SDK
target_link_libraries(test_mavlink_client_x86 PRIVATE
    ${FALCONMIND_SDK}
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(test_mavlink_client_x86 PRIVATE pthread)
endif()
