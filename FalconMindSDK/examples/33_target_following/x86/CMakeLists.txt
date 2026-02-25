cmake_minimum_required(VERSION 3.16)
project(33_target_following_x86 LANGUAGES CXX)

find_package(Eigen3 REQUIRED)
project(33_target_following_x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# FalconMindSDK路径配置
set(FALCONMIND_SDK_DIR "/home/shook/study/opencode/FalconMindSDK")
set(FALCONMIND_SDK_INSTALL "${FALCONMIND_SDK_DIR}/install/x86")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_INSTALL}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_INSTALL}/lib/libfalconmind_sdk.a")

# 检查SDK库是否存在
if(NOT EXISTS "${FALCONMIND_SDK_LIB}")
    message(FATAL_ERROR "FalconMindSDK库未找到: ${FALCONMIND_SDK_LIB}")
endif()

# 源文件
set(SOURCES src/main.cpp)

# 可执行文件
add_executable(33_target_following_x86 ${SOURCES})

# 头文件路径
target_include_directories(33_target_following_x86 PRIVATE
    ${EIGEN3_INCLUDE_DIR}
    ${FALCONMIND_SDK_INCLUDE}
)

# 链接库
target_link_libraries(33_target_following_x86 PRIVATE
    ${FALCONMIND_SDK_LIB}
    pthread
)

# 编译选项
target_compile_options(33_target_following_x86 PRIVATE
    -Wall
    -Wextra
    -O3
)

# 安装
install(TARGETS 33_target_following_x86
    RUNTIME DESTINATION bin
)
