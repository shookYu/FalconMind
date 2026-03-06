cmake_minimum_required(VERSION 3.10)
project(data_logger_process)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/dds_types
    ${OpenCV_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    data_logger_process.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    fastcdr
    fastrtps
    yaml-cpp
    paho-mqttpp3
    sqlite3
    ${OpenCV_LIBS}
    pthread
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES data_logger.yaml
    DESTINATION etc/falconmind
)
