# SdkInterface Adapter CMakeLists.txt
# 将 SDK 能力导出为 C 接口共享库

cmake_minimum_required(VERSION 3.16)

project(FalconMindSdkInterface VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译为共享库
add_library(falconmind_sdk_interface SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/adapter/SdkInterfaceImpl.cpp
)

# 设置输出名称
set_target_properties(falconmind_sdk_interface PROPERTIES
    OUTPUT_NAME "falconmind_sdk"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../NodeAgent/include/nodeagent/sdk/SdkInterface.h"
)

# 包含目录
target_include_directories(falconmind_sdk_interface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../NodeAgent/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 链接 SDK 库（内部使用）
target_link_libraries(falconmind_sdk_interface
    PRIVATE
        falconmind_sdk  # 链接原始 SDK 库
)

# 安装目标
install(TARGETS falconmind_sdk_interface
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/nodeagent/sdk
)
