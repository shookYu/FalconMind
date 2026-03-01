# 场景1.1 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(scenario_01_single_lawn_mower)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 FalconMindSDK
set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "FalconMindSDK根目录")
set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")

# 查找线程库
find_package(Threads REQUIRED)

# 源文件列表
set(SOURCES
    main.cpp
    LawnMowerScenario.cpp
)

# 头文件列表（用于IDE）
set(HEADERS
    LawnMowerScenario.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
)

# 库目录
target_link_directories(${PROJECT_NAME} PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O2
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# ============================================
# 真实飞控连接版本
# ============================================

# 真实版本源文件
set(REAL_SOURCES
    main_real.cpp
    LawnMowerScenarioReal.cpp
)

set(REAL_HEADERS
    LawnMowerScenarioReal.h
)

# 创建真实版本可执行文件
add_executable(${PROJECT_NAME}_real ${REAL_SOURCES} ${REAL_HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME}_real PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
)

# 库目录
target_link_directories(${PROJECT_NAME}_real PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME}_real PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

# 编译选项
target_compile_options(${PROJECT_NAME}_real PRIVATE
    -Wall
    -Wextra
    -O2
)

# 安装规则
install(TARGETS ${PROJECT_NAME}_real
    RUNTIME DESTINATION bin
)
