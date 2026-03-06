# FalconMind DDS - CMake Build Configuration
# Fast DDS IDL compilation and library generation

cmake_minimum_required(VERSION 3.16)
project(FalconMindDDS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Find Dependencies
# =============================================================================

find_package(fastcdr REQUIRED)
find_package(fastrtps REQUIRED)
find_package(foonathan_memory REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(OpenSSL REQUIRED)

# =============================================================================
# IDL Generation
# =============================================================================

set(IDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/idl/FalconMindTypes.idl)
set(IDL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${IDL_OUTPUT_DIR})

# Find fastddsgen
find_program(FASTDDSGEN fastddsgen 
    PATHS /usr/bin /usr/local/bin
    DOC "Fast DDS IDL generator"
)

if(NOT FASTDDSGEN)
    message(FATAL_ERROR "fastddsgen not found! Install Fast-DDS.")
endif()

# Generate C++ code from IDL
set(GENERATED_SOURCES
    ${IDL_OUTPUT_DIR}/FalconMindTypes.cxx
    ${IDL_OUTPUT_DIR}/FalconMindTypesPubSubTypes.cxx
)

set(GENERATED_HEADERS
    ${IDL_OUTPUT_DIR}/FalconMindTypes.h
    ${IDL_OUTPUT_DIR}/FalconMindTypesPubSubTypes.h
)

add_custom_command(
    OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
    COMMAND ${FASTDDSGEN}
        -d ${IDL_OUTPUT_DIR}
        -replace
        -typeobject
        ${IDL_FILE}
    DEPENDS ${IDL_FILE}
    COMMENT "Generating Fast DDS types from IDL"
)

add_custom_target(generate_dds_types ALL
    DEPENDS ${GENERATED_SOURCES} ${GENERATED_HEADERS}
)

# =============================================================================
# DDS Types Library
# =============================================================================

add_library(falconmind_dds_types SHARED
    ${GENERATED_SOURCES}
)

add_dependencies(falconmind_dds_types generate_dds_types)

target_include_directories(falconmind_dds_types
    PUBLIC
        $<BUILD_INTERFACE:${IDL_OUTPUT_DIR}>
        $<INSTALL_INTERFACE:include>
        ${fastcdr_INCLUDE_DIR}
        ${fastrtps_INCLUDE_DIR}
)

target_link_libraries(falconmind_dds_types
    PUBLIC
        fastcdr
        fastrtps
)

set_target_properties(falconmind_dds_types PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME DDS_Types
)

# =============================================================================
# DDS Participant Library (Shared DDS infrastructure)
# =============================================================================

add_library(falconmind_dds SHARED
    src/DDSParticipant.cpp
    src/DDSPublisher.cpp
    src/DDSSubscriber.cpp
    src/QoSConfig.cpp
)

target_include_directories(falconmind_dds
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${IDL_OUTPUT_DIR}
)

target_link_libraries(falconmind_dds
    PUBLIC
        falconmind_dds_types
        fastcdr
        fastrtps
        foonathan_memory
        TinyXML2::tinyxml2
        OpenSSL::SSL
)

set_target_properties(falconmind_dds PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME DDS
)

# =============================================================================
# Participant Discovery Server (optional, for large swarms)
# =============================================================================

add_executable(falconmind_discovery_server
    src/DiscoveryServer.cpp
)

target_link_libraries(falconmind_discovery_server
    falconmind_dds
    fastrtps
)

# =============================================================================
# DDS Configuration Files
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/dds_profile.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/config/dds_profile.xml
    @ONLY
)

# =============================================================================
# Installation
# =============================================================================

include(GNUInstallDirs)

install(TARGETS falconmind_dds_types falconmind_dds falconmind_discovery_server
    EXPORT FalconMindDDSTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${GENERATED_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/falconmind/dds
)

install(FILES
    include/falconmind/dds/DDSParticipant.h
    include/falconmind/dds/DDSPublisher.h
    include/falconmind/dds/DDSSubscriber.h
    include/falconmind/dds/QoSConfig.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/falconmind/dds
)

install(EXPORT FalconMindDDSTargets
    FILE FalconMindDDSTargets.cmake
    NAMESPACE FalconMind::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FalconMindDDS
)

# =============================================================================
# Package Configuration
# =============================================================================

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FalconMindDDSConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/FalconMindDDSConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FalconMindDDS
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/FalconMindDDSConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FalconMindDDSConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FalconMindDDSConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FalconMindDDS
)

# =============================================================================
# Docker Support
# =============================================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/docker/Dockerfile.fastdds.in
    ${CMAKE_CURRENT_BINARY_DIR}/docker/Dockerfile.fastdds
    @ONLY
)

# =============================================================================
# Testing
# =============================================================================

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
