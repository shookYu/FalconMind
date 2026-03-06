cmake_minimum_required(VERSION 3.10)
project(flight_control_process)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)
find_package(yaml-cpp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/dds_types
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/mavlink/include
)

# Source files
set(SOURCES
    flight_control_process.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    fastcdr
    fastrtps
    yaml-cpp
    pthread
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES flight_control.yaml
    DESTINATION etc/falconmind
)
