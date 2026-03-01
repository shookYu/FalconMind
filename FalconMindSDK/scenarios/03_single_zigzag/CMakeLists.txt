cmake_minimum_required(VERSION 3.16)
project(FalconMindSDK_scenario_03_single_zigzag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 FalconMindSDK
set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")

# 查找线程库
find_package(Threads REQUIRED)

set(SRCS
    ZigzagScenario.cpp
    main.cpp
)

add_executable(scenario_03_single_zigzag ${SRCS})

target_include_directories(scenario_03_single_zigzag PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
)

target_link_directories(scenario_03_single_zigzag PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(scenario_03_single_zigzag PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

target_compile_options(scenario_03_single_zigzag PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS scenario_03_single_zigzag
    RUNTIME DESTINATION bin
)
