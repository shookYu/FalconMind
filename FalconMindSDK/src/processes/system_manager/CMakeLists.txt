cmake_minimum_required(VERSION 3.10)
project(system_manager_process)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(yaml-cpp REQUIRED)
find_package(PahoMqttCpp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Source files
set(SOURCES
    system_manager_process.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    yaml-cpp
    paho-mqttpp3
    pthread
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES system_manager.yaml
    DESTINATION etc/falconmind
)
