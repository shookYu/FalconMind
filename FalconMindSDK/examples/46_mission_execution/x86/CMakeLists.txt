cmake_minimum_required(VERSION 3.16)
project(46_mission_execution_x86)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 FalconMindSDK
set(FALCONMIND_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_ROOT}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_ROOT}/install/x86/lib")

# 包含目录
include_directories(${FALCONMIND_SDK_INCLUDE})

# 链接目录
link_directories(${FALCONMIND_SDK_LIB})

# 可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    falconmind_sdk
    pthread
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
