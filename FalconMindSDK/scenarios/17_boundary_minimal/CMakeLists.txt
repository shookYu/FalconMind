# 场景 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(FalconMindScenario)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 FalconMindSDK
find_package(FalconMindSDK QUIET)
if(NOT FalconMindSDK_FOUND)
    # 使用相对路径
    set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../")
    set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
    set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")
endif()

# 查找依赖
find_package(Threads REQUIRED)

# 创建可执行文件
get_filename_component(SCENARIO_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME "scenario_${SCENARIO_NAME}")

add_executable(${TARGET_NAME}
    main.cpp
)

target_include_directories(${TARGET_NAME} PRIVATE
    ${FALCONMINDSDK_INCLUDE_DIR}
    /usr/include
)

target_link_directories(${TARGET_NAME} PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(${TARGET_NAME} PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

# 编译选项
target_compile_options(${TARGET_NAME} PRIVATE
    -Wall
    -Wextra
    -O2
)

# 安装规则
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION bin
)
