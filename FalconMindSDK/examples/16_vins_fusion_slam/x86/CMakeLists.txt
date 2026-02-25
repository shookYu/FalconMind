cmake_minimum_required(VERSION 3.16)
project(16_vins_fusion_slam_x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

# Find dependencies
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

set(FALCONMIND_SDK "/home/shook/study/opencode/FalconMindSDK")

add_executable(16_vins_fusion_slam_x86 src/main.cpp)

target_include_directories(16_vins_fusion_slam_x86 PRIVATE 
    ${FALCONMIND_SDK}/include
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(16_vins_fusion_slam_x86 PRIVATE 
    ${FALCONMIND_SDK}/build/libfalconmind_sdk.a
    ${OpenCV_LIBS}
    pthread
)
