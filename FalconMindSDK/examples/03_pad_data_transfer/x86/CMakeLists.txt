cmake_minimum_required(VERSION 3.16)
project(03_pad_data_transfer_x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# FalconMindSDK路径配置 (引用install/x86目录)
set(FALCONMIND_SDK_DIR "/home/shook/study/opencode/FalconMindSDK")
set(FALCONMIND_SDK_INSTALL "${FALCONMIND_SDK_DIR}/install/x86")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_INSTALL}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_INSTALL}/lib/libfalconmind_sdk.a")

# 检查SDK库是否存在
if(NOT EXISTS "${FALCONMIND_SDK_LIB}")
    message(FATAL_ERROR "FalconMindSDK库未找到: ${FALCONMIND_SDK_LIB}")
endif()

# 检查ONNX Runtime
find_library(ONNXRUNTIME_LIB onnxruntime PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h PATHS /usr/include /usr/local/include)

if(ONNXRUNTIME_LIB AND ONNXRUNTIME_INCLUDE)
    message(STATUS "ONNX Runtime: Found system library")
    set(ONNXRUNTIME_FOUND TRUE)
else()
    message(WARNING "ONNX Runtime not found. Install: sudo apt-get install onnxruntime")
    set(ONNXRUNTIME_FOUND FALSE)
endif()

# 源文件
set(SOURCES
    src/main.cpp
)

# 可执行文件
add_executable(03_pad_data_transfer_x86 ${SOURCES})

# 头文件路径
target_include_directories(03_pad_data_transfer_x86 PRIVATE
    ${FALCONMIND_SDK_INCLUDE}
)

# 链接库
target_link_libraries(03_pad_data_transfer_x86 PRIVATE
    ${FALCONMIND_SDK_LIB}
    pthread
)

if(ONNXRUNTIME_FOUND)
    target_include_directories(03_pad_data_transfer_x86 PRIVATE ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(03_pad_data_transfer_x86 PRIVATE ${ONNXRUNTIME_LIB})
endif()

# 编译选项
target_compile_options(03_pad_data_transfer_x86 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O3
)

# 安装
install(TARGETS 03_pad_data_transfer_x86
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "=== FalconMindSDK 示例03 x86平台配置 ===")
message(STATUS "  SDK安装目录: ${FALCONMIND_SDK_INSTALL}")
message(STATUS "  头文件目录: ${FALCONMIND_SDK_INCLUDE}")
message(STATUS "  库文件目录: ${FALCONMIND_SDK_LIB}")
message(STATUS "  可执行文件: 03_pad_data_transfer_x86")
message(STATUS "")
