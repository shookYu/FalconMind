cmake_minimum_required(VERSION 3.10)
project(mission_planner_process)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PahoMqttCpp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/dds_types
)

# Source files
set(SOURCES
    mission_planner_process.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    fastcdr
    fastrtps
    yaml-cpp
    paho-mqttpp3
    pthread
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES mission_planner.yaml
    DESTINATION etc/falconmind
)
