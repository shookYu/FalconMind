# aarch64-linux-gnu 交叉编译工具链配置
#
# 工具链路径: /opt/FriendlyARM/toolchain/11.3-aarch64
#

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 交叉编译工具链路径
set(TOOLCHAIN_PREFIX /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}ld)
set(CMAKE_NM ${TOOLCHAIN_PREFIX}nm)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}ranlib)

# Sysroot
set(CMAKE_SYSROOT /opt/FriendlyARM/toolchain/11.3-aarch64/aarch64-cortexa53-linux-gnu/sysroot)

# arm64依赖库头文件路径
set(ARM64_DEPEND_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/3rd/install/arm64/include")

# 查找程序
find_program(CMAKE_C_COMPILER_NAMES ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER_NAMES ${TOOLCHAIN_PREFIX}g++)

# 搜索路径配置
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 添加arm64依赖头文件路径
list(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${ARM64_DEPEND_INCLUDE}")

# 编译选项优化
set(CMAKE_C_FLAGS_INIT "-march=armv8-a -O3")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a -O3")
