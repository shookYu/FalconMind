cmake_minimum_required(VERSION 3.16)
project(scenario_08_single_low_battery)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FALCONMINDSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FALCONMINDSDK_INCLUDE_DIR "${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "${FALCONMINDSDK_ROOT}/install/x86/lib")

find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

# 包含common目录下的RealScenarioBase.cpp
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../common/RealScenarioBase.cpp")

add_executable(scenario_08_single_low_battery ${SOURCES})

target_include_directories(scenario_08_single_low_battery PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMINDSDK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(scenario_08_single_low_battery PRIVATE
    ${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(scenario_08_single_low_battery PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

target_compile_options(scenario_08_single_low_battery PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS scenario_08_single_low_battery
    RUNTIME DESTINATION bin
)
