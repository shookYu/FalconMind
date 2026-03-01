cmake_minimum_required(VERSION 3.16)
project(scenario_20_e2e_multi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")

find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

# 包含common目录下的RealScenarioBase.cpp
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../common/RealScenarioBase.cpp")

add_executable(scenario_20_e2e_multi ${SOURCES})

target_include_directories(scenario_20_e2e_multi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(scenario_20_e2e_multi PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(scenario_20_e2e_multi PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

target_compile_options(scenario_20_e2e_multi PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS scenario_20_e2e_multi
    RUNTIME DESTINATION bin
)
