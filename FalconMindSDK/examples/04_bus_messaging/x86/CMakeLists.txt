cmake_minimum_required(VERSION 3.16)
project(04_bus_messaging_x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

set(FALCONMIND_SDK_DIR "/home/shook/study/opencode/FalconMindSDK")
set(FALCONMIND_SDK_INSTALL "${FALCONMIND_SDK_DIR}/install/x86")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_INSTALL}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_INSTALL}/lib/libfalconmind_sdk.a")

if(NOT EXISTS "${FALCONMIND_SDK_LIB}")
    message(FATAL_ERROR "FalconMindSDK库未找到: ${FALCONMIND_SDK_LIB}")
endif()

find_library(ONNXRUNTIME_LIB onnxruntime PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h PATHS /usr/include /usr/local/include)

if(ONNXRUNTIME_LIB AND ONNXRUNTIME_INCLUDE)
    message(STATUS "ONNX Runtime: Found system library")
    set(ONNXRUNTIME_FOUND TRUE)
else()
    message(WARNING "ONNX Runtime not found.")
    set(ONNXRUNTIME_FOUND FALSE)
endif()

set(SOURCES src/main.cpp)
add_executable(04_bus_messaging_x86 ${SOURCES})

target_include_directories(04_bus_messaging_x86 PRIVATE ${FALCONMIND_SDK_INCLUDE})
target_link_libraries(04_bus_messaging_x86 PRIVATE ${FALCONMIND_SDK_LIB} pthread)

if(ONNXRUNTIME_FOUND)
    target_include_directories(04_bus_messaging_x86 PRIVATE ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(04_bus_messaging_x86 PRIVATE ${ONNXRUNTIME_LIB})
endif()

target_compile_options(04_bus_messaging_x86 PRIVATE -Wall -Wextra -Wpedantic -O3)

install(TARGETS 04_bus_messaging_x86 RUNTIME DESTINATION bin)

message(STATUS "")
message(STATUS "=== FalconMindSDK 示例04 x86平台配置 ===")
message(STATUS "  SDK安装目录: ${FALCONMIND_SDK_INSTALL}")
message(STATUS "  可执行文件: 04_bus_messaging_x86")
message(STATUS "")
