# RV1126B 交叉编译工具链配置
#
# RV1126B 平台规格:
# - CPU: 四核Cortex-A53 (64-bit)
# - NPU: 3.0 TOPS
# - 工艺: 22nm
# - 架构: aarch64 (64位ARM)
#
# 工具链: /opt/FriendlyARM/toolchain/11.3-aarch64
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

# 查找程序
find_program(CMAKE_C_COMPILER_NAMES ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER_NAMES ${TOOLCHAIN_PREFIX}g++)

# 搜索路径配置
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 编译选项优化 (开启NEON、Crypto和Cortex-A53优化)
set(CMAKE_C_FLAGS_INIT "-march=armv8-a+crc+crypto -mtune=cortex-a53 -O3")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a+crc+crypto -mtune=cortex-a53 -O3")
