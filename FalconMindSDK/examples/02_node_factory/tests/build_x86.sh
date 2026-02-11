#!/bin/bash
# FalconMindSDK 示例02 - x86平台构建脚本
#
# 功能:
# - 本地编译x86版本
# - 编译产物输出到x86/build目录
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"
SDK_DIR="$(dirname "$EXAMPLE_DIR")/.."  # SDK在FalconMindSDK根目录
PLATFORM_DIR="$EXAMPLE_DIR/x86"
BUILD_DIR="$PLATFORM_DIR/build"

echo "================================================================================"
echo "              FalconMindSDK 示例02: NodeFactory节点工厂 (x86构建)"
echo "================================================================================"
echo ""
echo "平台目录: $PLATFORM_DIR"
echo "构建目径: $BUILD_DIR"
echo ""

# 检查SDK库是否存在
SDK_LIB="$SDK_DIR/build/libfalconmind_sdk.a"
if [ ! -f "$SDK_LIB" ]; then
    echo "[警告] SDK库不存在: $SDK_LIB"
    echo "[提示] 请先构建SDK: cd $SDK_DIR && mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

echo "[1] 创建构建目录: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[2] 配置CMake (x86本地编译)"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$SDK_DIR/build" \
      "$PLATFORM_DIR"

echo "[3] 编译项目"
make -j$(nproc)

echo ""
echo "[4] 运行测试"
if [ -f "./02_node_factory_x86" ]; then
    echo "执行程序:"
    ./02_node_factory_x86
    echo ""
    echo "================================================================================"
    echo "                    x86平台构建测试完成"
    echo "================================================================================"
    echo "可执行文件: $BUILD_DIR/02_node_factory_x86"
else
    echo "[错误] 编译产物不存在"
    exit 1
fi
