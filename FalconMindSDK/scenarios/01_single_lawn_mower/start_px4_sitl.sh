#!/bin/bash
# start_px4_sitl.sh - 启动PX4 SITL仿真环境

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "PX4 SITL 启动脚本"
echo "=========================================="
echo ""

# 检查PX4-Autopilot目录
PX4_DIR="${HOME}/PX4-Autopilot"

if [ ! -d "$PX4_DIR" ]; then
    echo -e "${RED}错误: 未找到 PX4-Autopilot 目录${NC}"
    echo ""
    echo "请先安装 PX4:"
    echo "  cd ~"
    echo "  git clone https://github.com/PX4/PX4-Autopilot.git --recursive"
    echo "  cd PX4-Autopilot"
    echo "  bash ./Tools/setup/ubuntu.sh"
    echo "  make px4_sitl_default"
    echo ""
    exit 1
fi

echo "找到 PX4-Autopilot: $PX4_DIR"
echo ""

# 检查必要的工具
echo "检查依赖..."

if ! command -v gazebo &> /dev/null; then
    echo -e "${YELLOW}警告: 未找到 Gazebo${NC}"
    echo "PX4 SITL 将使用无头模式（无GUI）启动"
    echo ""
    HEADLESS=1
else
    HEADLESS=0
fi

# 启动PX4 SITL
echo "启动 PX4 SITL..."
echo ""

cd "$PX4_DIR"

if [ "$HEADLESS" -eq 1 ]; then
    echo "模式: 无头模式（Headless）"
    echo "MAVLink端口: 14550"
    echo ""
    echo "按 Ctrl+C 停止"
    echo ""
    make px4_sitl_default none
else
    echo "模式: Gazebo仿真"
    echo "MAVLink端口: 14550"
    echo ""
    echo "按 Ctrl+C 停止"
    echo ""
    make px4_sitl_default gazebo
fi
