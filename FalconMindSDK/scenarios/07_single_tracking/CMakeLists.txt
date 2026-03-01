cmake_minimum_required(VERSION 3.16)
project(scenario_07_single_tracking)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")

find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

# 包含common目录下的RealScenarioBase.cpp
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../common/RealScenarioBase.cpp")

add_executable(scenario_07_single_tracking ${SOURCES})

target_include_directories(scenario_07_single_tracking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(scenario_07_single_tracking PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(scenario_07_single_tracking PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

target_compile_options(scenario_07_single_tracking PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS scenario_07_single_tracking
    RUNTIME DESTINATION bin
)
