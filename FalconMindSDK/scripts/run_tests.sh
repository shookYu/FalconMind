#!/usr/bin/env bash
# FalconMindSDK - 一键运行所有单元/集成测试
# 用法: ./scripts/run_tests.sh [build_dir]
# 若未指定 build_dir，则使用当前目录下的 build；若在 repo 根目录则使用 FalconMindSDK/build。

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$SDK_ROOT/build}"

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Build directory not found: $BUILD_DIR"
    echo "Run from SDK root: mkdir build && cd build && cmake .. -DFALCONMINDSDK_BUILD_TESTS=ON && make -j"
    exit 1
fi

TESTS=(
    falconmind_sdk_core_tests
    falconmind_flow_executor_tests
    falconmind_node_factory_tests
    falconmind_detection_packet_tests
    falconmind_yolo_prepost_tests
    falconmind_tracker_tests
    falconmind_pipeline_link_tests
    falconmind_flow_executor_e2e_tests
)

PASS=0
FAIL=0
for t in "${TESTS[@]}"; do
    exe="$BUILD_DIR/$t"
    if [[ ! -x "$exe" ]]; then
        echo "SKIP $t (not built)"
        continue
    fi
    if "$exe" >/dev/null 2>&1; then
        echo "PASS $t"
        ((PASS++)) || true
    else
        echo "FAIL $t"
        ((FAIL++)) || true
    fi
done

echo "---"
echo "Passed: $PASS, Failed: $FAIL"
if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
exit 0
