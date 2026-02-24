cmake_minimum_required(VERSION 3.16)
project(VinsFusionIntegration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FalconMind SDK
set(FALCONMIND_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Find dependencies
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

# VINS-Fusion paths (user needs to set these)
set(VINS_FUSION_ROOT "$ENV{VINS_FUSION_ROOT}" CACHE PATH "Path to VINS-Fusion source")

if(NOT VINS_FUSION_ROOT)
    message(WARNING "VINS_FUSION_ROOT not set. Please set environment variable:\n"
                    "  export VINS_FUSION_ROOT=/path/to/VINS-Fusion\n"
                    "Example will compile with stub implementation.")
endif()

# Create library
add_library(vins_fusion_adapter INTERFACE)
target_include_directories(vins_fusion_adapter INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FALCONMIND_SDK}/include
)

if(VINS_FUSION_ROOT)
    target_include_directories(vins_fusion_adapter INTERFACE
        ${VINS_FUSION_ROOT}/vins_estimator/src
        ${VINS_FUSION_ROOT}/camera_model/include
    )
    target_compile_definitions(vins_fusion_adapter INTERFACE 
        VINS_FUSION_ENABLED=1
    )
endif()

target_link_libraries(vins_fusion_adapter INTERFACE
    ${OpenCV_LIBS}
    Eigen3::Eigen
)

# Installation
install(FILES VinsFusionAdapter.h
    DESTINATION include/falconmind/3rd/vins_fusion
)
