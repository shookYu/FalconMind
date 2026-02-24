cmake_minimum_required(VERSION 3.16)
project(28_mavlink_sensors_x86)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDK paths
set(FALCONMIND_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../install/x86)
set(FALCONMIND_SDK_INCLUDE ${FALCONMIND_SDK_ROOT}/include)
set(FALCONMIND_SDK_LIB ${FALCONMIND_SDK_ROOT}/lib)

include_directories(${FALCONMIND_SDK_INCLUDE})
link_directories(${FALCONMIND_SDK_LIB})

add_executable(${PROJECT_NAME} main.cpp)
add_executable(test_imu test_imu.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    falconmind_sdk
    pthread
)

target_link_libraries(test_imu PRIVATE
    falconmind_sdk
    pthread
)
