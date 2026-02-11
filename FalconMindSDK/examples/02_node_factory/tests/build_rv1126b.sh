#!/bin/bash
# FalconMindSDK 示例02 - RV1126B平台交叉编译脚本
#
# 功能:
# - 交叉编译RV1126B版本
# - 编译产物输出到rv1126b/build目录
# - 需要安装aarch64-linux-gnu工具链
#
# RV1126B平台规格:
# - CPU: 四核Cortex-A53 (64-bit)
# - NPU: 3.0 TOPS (INT8/INT16)
# - 工艺: 22nm
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"
SDK_DIR="$(dirname "$EXAMPLE_DIR")/.."  # SDK在FalconMindSDK根目录
PLATFORM_DIR="$EXAMPLE_DIR/rv1126b"
BUILD_DIR="$PLATFORM_DIR/build"

echo "================================================================================"
echo "              FalconMindSDK 示例02: NodeFactory节点工厂 (RV1126B交叉编译)"
echo "================================================================================"
echo ""
echo "平台目录: $PLATFORM_DIR"
echo "构建目录: $BUILD_DIR"
echo ""

# 检查工具链
if ! command -v aarch64-linux-gnu-gcc &> /dev/null; then
    echo "[错误] 未找到aarch64-linux-gnu工具链"
    echo "[提示] 请安装: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
    exit 1
fi

# 检查SDK库是否存在 (需要arm64版本SDK)
SDK_LIB="$SDK_DIR/build_arm64/lib/libfalconmind_sdk.a"
if [ ! -f "$SDK_LIB" ]; then
    echo "[警告] SDK arm64库不存在: $SDK_LIB"
    echo "[提示] 请先构建SDK: cd $SDK_DIR && mkdir -p build_arm64 && cd build_arm64 && cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake .. && make"
    exit 1
fi

echo "[1] 工具链版本"
aarch64-linux-gnu-gcc --version | head -1

echo "[2] 创建构建目录: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[3] 配置CMake (RV1126B交叉编译)"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$PLATFORM_DIR/toolchain.cmake" \
      -DSDK_ROOT="$SDK_DIR/build_arm64" \
      "$PLATFORM_DIR"

echo "[4] 交叉编译项目"
make -j$(nproc)

echo ""
echo "[5] 生成可执行文件"
if [ -f "./02_node_factory_rv1126b" ]; then
    echo "可执行文件: $BUILD_DIR/02_node_factory_rv1126b"
    file ./02_node_factory_rv1126b
    echo ""
    echo "================================================================================"
    echo "                    RV1126B交叉编译完成"
    echo "================================================================================"
    echo "[提示] 请将可执行文件传输到RV1126B设备上运行"
else
    echo "[错误] 编译产物不存在"
    exit 1
fi
