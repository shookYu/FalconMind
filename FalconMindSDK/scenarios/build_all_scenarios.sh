#!/bin/bash
# build_all_scenarios.sh - 编译所有场景案例

set -e

SCENARIOS_DIR="$(cd "$(dirname "$0")" && pwd)"
FALCONMINDSDK_ROOT="${SCENARIOS_DIR}/.."
BUILD_DIR="${SCENARIOS_DIR}/build"
INSTALL_PREFIX="${BUILD_DIR}/install"

echo "================================================================================"
echo "FalconMindSDK 场景案例编译脚本"
echo "================================================================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 编译统计
TOTAL=0
SUCCESS=0
FAILED=0

# 创建构建目录
mkdir -p "${BUILD_DIR}"

# 编译单个场景的函数
build_scenario() {
    local scenario_dir=$1
    local scenario_name=$(basename "$scenario_dir")
    local scenario_build_dir="${BUILD_DIR}/${scenario_name}"
    
    echo "编译场景: ${scenario_name}..."
    
    mkdir -p "${scenario_build_dir}"
    cd "${scenario_build_dir}"
    
    # 运行CMake
    if cmake "${scenario_dir}" \
        -DFALCONMINDSDK_ROOT="${FALCONMINDSDK_ROOT}" \
        -DFALCONMINDSDK_INCLUDE_DIR="${FALCONMINDSDK_ROOT}/include" \
        -DFALCONMINDSDK_LIB_DIR="${FALCONMINDSDK_ROOT}/install/x86/lib" \
        -DCMAKE_BUILD_TYPE=Release \
        > cmake.log 2>&1; then
        
        # 编译
        if make -j4 > make.log 2>&1; then
            echo -e "${GREEN}✓${NC} ${scenario_name} 编译成功"
            ((SUCCESS++))
            return 0
        else
            echo -e "${RED}✗${NC} ${scenario_name} 编译失败 (make错误)"
            ((FAILED++))
            return 1
        fi
    else
        echo -e "${RED}✗${NC} ${scenario_name} 编译失败 (cmake错误)"
        ((FAILED++))
        return 1
    fi
}

# 遍历所有场景目录
echo "开始编译场景..."
echo ""

for scenario_dir in "${SCENARIOS_DIR}"/*/; do
    if [ -f "${scenario_dir}/main.cpp" ]; then
        ((TOTAL++))
        build_scenario "$scenario_dir" || true
    fi
done

echo ""
echo "================================================================================"
echo "编译统计"
echo "================================================================================"
echo -e "总场景数: ${TOTAL}"
echo -e "编译成功: ${GREEN}${SUCCESS}${NC}"
echo -e "编译失败: ${RED}${FAILED}${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}所有场景编译成功!${NC}"
    exit 0
else
    echo -e "${YELLOW}有 ${FAILED} 个场景编译失败，请查看日志${NC}"
    exit 1
fi
