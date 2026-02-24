# Toolchain file for cross-compiling to ARM64 (RK3588/RK3576/RV1126B)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Target environment
set(CMAKE_FIND_ROOT_PATH 
    /usr/aarch64-linux-gnu
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../install/arm64
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Compiler flags for RK3588
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -mtune=cortex-a76" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mtune=cortex-a76" CACHE STRING "" FORCE)

# RKNN backend enabled
set(FALCONMINDSDK_RKNN_BACKEND_ENABLED 1)
