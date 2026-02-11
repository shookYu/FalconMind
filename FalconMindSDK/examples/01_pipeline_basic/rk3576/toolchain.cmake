# RK3576 交叉编译工具链配置
#
# RK3576 平台规格:
# - CPU: 四核Cortex-A76 @2.2GHz + 四核Cortex-A55 @1.8GHz (8核心)
# - NPU: 6 TOPS (支持INT4/INT8/INT16)
# - GPU: Mali-G52 MP2
# - 编解码: 4K@60fps H.265, 4K@60fps VP9
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

# 搜索路径
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 编译选项优化 (big.LITTLE架构优化)
set(CMAKE_C_FLAGS_INIT "-march=armv8-a+crc+crypto -mtune=cortex-a75 -O3")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a+crc+crypto -mtune=cortex-a75 -O3")
