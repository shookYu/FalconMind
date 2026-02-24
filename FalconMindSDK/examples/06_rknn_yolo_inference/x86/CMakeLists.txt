cmake_minimum_required(VERSION 3.16)
project(06_rknn_yolo_inference_x86 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# FalconMindSDK路径配置
set(FALCONMIND_SDK_DIR "/home/shook/study/opencode/FalconMindSDK")
set(FALCONMIND_SDK_INSTALL "${FALCONMIND_SDK_DIR}/install/x86")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_INSTALL}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_INSTALL}/lib/libfalconmind_sdk.a")

if(NOT EXISTS "${FALCONMIND_SDK_LIB}")
    message(FATAL_ERROR "FalconMindSDK库未找到: ${FALCONMIND_SDK_LIB}")
endif()

# ONNX Runtime (1.18.0 支持opset 21)
set(ONNXRUNTIME_DIR "/tmp/onnxruntime-linux-x64-1.18.0")
set(ONNXRUNTIME_LIB "${ONNXRUNTIME_DIR}/lib/libonnxruntime.so")
set(ONNXRUNTIME_INCLUDE "${ONNXRUNTIME_DIR}/include")

if(EXISTS "${ONNXRUNTIME_LIB}" AND EXISTS "${ONNXRUNTIME_INCLUDE}/onnxruntime_cxx_api.h}")
    message(STATUS "ONNX Runtime: Found (${ONNXRUNTIME_LIB})")
    set(ONNXRUNTIME_FOUND TRUE)
    add_definitions(-DFALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED=1)
else()
    message(WARNING "ONNX Runtime not found at ${ONNXRUNTIME_DIR}")
    set(ONNXRUNTIME_FOUND FALSE)
endif()

# 源文件
set(SOURCES src/main.cpp)
add_executable(06_rknn_yolo_inference_x86 ${SOURCES})

# 头文件路径
target_include_directories(06_rknn_yolo_inference_x86 PRIVATE 
    ${FALCONMIND_SDK_INCLUDE}
)

# 链接库
target_link_libraries(06_rknn_yolo_inference_x86 PRIVATE 
    ${FALCONMIND_SDK_LIB}
    pthread
)

if(ONNXRUNTIME_FOUND)
    target_include_directories(06_rknn_yolo_inference_x86 PRIVATE ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(06_rknn_yolo_inference_x86 PRIVATE ${ONNXRUNTIME_LIB})
else()
    message(FATAL_ERROR "ONNX Runtime not found, cannot compile")
endif()

target_link_libraries(06_rknn_yolo_inference_x86 PRIVATE curl)

# 编译选项
target_compile_options(06_rknn_yolo_inference_x86 PRIVATE
    -Wall
    -Wextra
    -O3
)

message(STATUS "")
message(STATUS "=== FalconMindSDK 示例06 x86平台配置 ===")
message(STATUS "  ONNX Runtime: " ${ONNXRUNTIME_FOUND})
message(STATUS "  可执行文件: 06_rknn_yolo_inference_x86")
message(STATUS "")
