#!/bin/bash
# FalconMindSDK 示例02 - 一键构建所有平台
#
# 功能:
# - 构建x86本地版本
# - 构建所有RK平台交叉编译版本(需要工具链)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================================================"
echo "              FalconMindSDK 示例02: NodeFactory节点工厂 (全平台构建)"
echo "================================================================================"
echo ""

# 构建x86版本
echo "[1] 构建x86版本"
echo "--------------------------------------------------------"
chmod +x "$SCRIPT_DIR/build_x86.sh"
"$SCRIPT_DIR/build_x86.sh"
echo ""

echo "[2] 构建RV1126B版本 (需要aarch64工具链)"
echo "--------------------------------------------------------"
chmod +x "$SCRIPT_DIR/build_rv1126b.sh"
"$SCRIPT_DIR/build_rv1126b.sh" || echo "[跳过] RV1126B工具链未安装"
echo ""

echo "[3] 构建RK3576版本 (需要aarch64工具链)"
echo "--------------------------------------------------------"
chmod +x "$SCRIPT_DIR/build_rk3576.sh"
"$SCRIPT_DIR/build_rk3576.sh" || echo "[跳过] RK3576工具链未安装"
echo ""

echo "[4] 构建RK3588版本 (需要aarch64工具链)"
echo "--------------------------------------------------------"
chmod +x "$SCRIPT_DIR/build_rk3588.sh"
"$SCRIPT_DIR/build_rk3588.sh" || echo "[跳过] RK3588工具链未安装"
echo ""

echo "================================================================================"
echo "                    所有平台构建完成!"
echo "================================================================================"
echo ""
echo "生成的可执行文件:"
echo "  - $EXAMPLE_DIR/x86/build/02_node_factory_x86 (x86)"
echo "  - $EXAMPLE_DIR/rv1126b/build/02_node_factory_rv1126b (RV1126B)"
echo "  - $EXAMPLE_DIR/rk3576/build/02_node_factory_rk3576 (RK3576)"
echo "  - $EXAMPLE_DIR/rk3588/build/02_node_factory_rk3588 (RK3588)"
echo ""
echo "注意: arm64平台需要先安装交叉编译工具链"
echo "  sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
