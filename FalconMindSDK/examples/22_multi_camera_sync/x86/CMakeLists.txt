cmake_minimum_required(VERSION 3.16)
project(22_multi_camera_sync_x86 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Find Eigen3
find_package(Eigen3 REQUIRED)

set(FALCONMIND_SDK "/home/shook/study/opencode/FalconMindSDK")

add_executable(22_multi_camera_sync_x86 src/main.cpp)

target_include_directories(22_multi_camera_sync_x86 PRIVATE
    ${FALCONMIND_SDK}/include
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(22_multi_camera_sync_x86 PRIVATE 
    ${FALCONMIND_SDK}/build/libfalconmind_sdk.a 
    pthread
)
