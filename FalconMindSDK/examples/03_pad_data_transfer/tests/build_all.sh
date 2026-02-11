#!/bin/bash
# 一键构建所有平台

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================================================"
echo "          FalconMindSDK 示例03: Pad数据传输 - 一键构建脚本"
echo "================================================================================"
echo ""

# 构建x86平台
echo "[1/4] 构建x86平台..."
cd "$EXAMPLE_DIR/x86"
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
echo "    x86平台构建成功"
echo ""

# 构建RK3588平台
echo "[2/4] 构建RK3588平台..."
cd "$EXAMPLE_DIR/rk3588"
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j$(nproc)
echo "    RK3588平台构建成功"
echo ""

# 构建RK3576平台
echo "[3/4] 构建RK3576平台..."
cd "$EXAMPLE_DIR/rk3576"
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j$(nproc)
echo "    RK3576平台构建成功"
echo ""

# 构建RV1126B平台
echo "[4/4] 构建RV1126B平台..."
cd "$EXAMPLE_DIR/rv1126b"
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j$(nproc)
echo "    RV1126B平台构建成功"
echo ""

echo "================================================================================"
echo "                    所有平台构建成功"
echo "================================================================================"
echo ""
echo "可执行文件:"
echo "  - $EXAMPLE_DIR/x86/build/03_pad_data_transfer_x86"
echo "  - $EXAMPLE_DIR/rk3588/build/03_pad_data_transfer_rk3588"
echo "  - $EXAMPLE_DIR/rk3576/build/03_pad_data_transfer_rk3576"
echo "  - $EXAMPLE_DIR/rv1126b/build/03_pad_data_transfer_rv1126b"
echo ""
