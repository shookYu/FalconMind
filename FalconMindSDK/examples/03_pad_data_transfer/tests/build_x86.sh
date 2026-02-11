#!/bin/bash
# x86平台构建脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================================================"
echo "          FalconMindSDK 示例03: Pad数据传输 - x86平台构建"
echo "================================================================================"
echo ""

cd "$EXAMPLE_DIR/x86"
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo ""
echo "================================================================================"
echo "                    x86平台构建成功"
echo "================================================================================"
echo ""
echo "可执行文件: $EXAMPLE_DIR/x86/build/03_pad_data_transfer_x86"
echo ""
