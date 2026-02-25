cmake_minimum_required(VERSION 3.16)
project(test_high_level_api_x86 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(FALCONMIND_SDK "/home/shook/study/opencode/FalconMindSDK")

add_executable(test_high_level_api_x86 src/main.cpp)

target_include_directories(test_high_level_api_x86 PRIVATE
    ${FALCONMIND_SDK}/include
)

target_link_libraries(test_high_level_api_x86 PRIVATE
    ${FALCONMIND_SDK}/build/libfalconmind_sdk.a
    pthread
)
