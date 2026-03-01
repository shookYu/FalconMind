# Simplified NodeAgent CMake for testing
cmake_minimum_required(VERSION 3.16)
project(NodeAgentTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# 模拟 SDK
add_library(mock_sdk SHARED MockSdkImpl.cpp)
target_include_directories(mock_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set_target_properties(mock_sdk PROPERTIES OUTPUT_NAME "falconmind_sdk")

# 测试程序
add_executable(test_sdk_loader 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/sdk/SdkLoader.cpp
    test_sdk_loader.cpp
)
target_include_directories(test_sdk_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
find_package(Threads REQUIRED)
target_link_libraries(test_sdk_loader PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
