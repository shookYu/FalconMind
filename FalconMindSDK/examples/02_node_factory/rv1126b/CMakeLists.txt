cmake_minimum_required(VERSION 3.16)
project(02_node_factory_rv1126b LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 交叉编译工具链配置
set(CMAKE_C_COMPILER /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-g++)
set(CMAKE_AR /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-ar)
set(CMAKE_LINKER /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-ld)
set(CMAKE_NM /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-nm)
set(CMAKE_OBJDUMP /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-objdump)
set(CMAKE_RANLIB /opt/FriendlyARM/toolchain/11.3-aarch64/bin/aarch64-cortexa53-linux-gnu-ranlib)
set(CMAKE_SYSROOT /opt/FriendlyARM/toolchain/11.3-aarch64/aarch64-cortexa53-linux-gnu/sysroot)

# FalconMindSDK安装路径 (引用install/arm64目录)
set(FALCONMIND_SDK_DIR "/home/shook/study/opencode/FalconMindSDK")
set(FALCONMIND_SDK_INSTALL "${FALCONMIND_SDK_DIR}/install/arm64")
set(FALCONMIND_SDK_INCLUDE "${FALCONMIND_SDK_INSTALL}/include")
set(FALCONMIND_SDK_LIB "${FALCONMIND_SDK_INSTALL}/lib/libfalconmind_sdk.a")

# 检查SDK库是否存在
if(NOT EXISTS "${FALCONMIND_SDK_LIB}")
    message(FATAL_ERROR "FalconMindSDK arm64库未找到: ${FALCONMIND_SDK_LIB}")
endif()

# 源文件
set(SOURCES src/main.cpp)

# 可执行文件
add_executable(02_node_factory_rv1126b ${SOURCES})

# 头文件路径
target_include_directories(02_node_factory_rv1126b PRIVATE ${FALCONMIND_SDK_INCLUDE})

# 链接库
target_link_libraries(02_node_factory_rv1126b PRIVATE
    ${FALCONMIND_SDK_LIB}
    pthread
)

# 编译选项
target_compile_options(02_node_factory_rv1126b PRIVATE
    -Wall
    -Wextra
    -O3
)

message(STATUS "")
message(STATUS "=== FalconMindSDK 示例02 RV1126B平台配置 ===")
message(STATUS "  SDK安装目录: ${FALCONMIND_SDK_INSTALL}")
message(STATUS "  头文件目录: ${FALCONMIND_SDK_INCLUDE}")
message(STATUS "  库文件目录: ${FALCONMIND_SDK_LIB}")
message(STATUS "  工具链: aarch64-cortexa53-linux-gnu")
message(STATUS "  可执行文件: 02_node_factory_rv1126b")
message(STATUS "")
