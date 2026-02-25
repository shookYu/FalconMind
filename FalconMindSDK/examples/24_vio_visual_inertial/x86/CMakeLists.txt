cmake_minimum_required(VERSION 3.16)
project(24_vio_visual_inertial_x86 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Eigen3 REQUIRED)

set(FALCONMIND_SDK "/home/shook/study/opencode/FalconMindSDK")

add_executable(24_vio_visual_inertial_x86 src/main.cpp)

target_include_directories(24_vio_visual_inertial_x86 PRIVATE
    ${FALCONMIND_SDK}/include
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(24_vio_visual_inertial_x86 PRIVATE
    ${FALCONMIND_SDK}/build/libfalconmind_sdk.a
    pthread
)
