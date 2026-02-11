#!/bin/bash
# RV1126B平台构建脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================================================"
echo "          FalconMindSDK 示例03: Pad数据传输 - RV1126B平台构建"
echo "================================================================================"
echo ""

cd "$EXAMPLE_DIR/rv1126b"
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j$(nproc)

echo ""
echo "================================================================================"
echo "                    RV1126B平台构建成功"
echo "================================================================================"
echo ""
echo "可执行文件: $EXAMPLE_DIR/rv1126b/build/03_pad_data_transfer_rv1126b"
echo ""
