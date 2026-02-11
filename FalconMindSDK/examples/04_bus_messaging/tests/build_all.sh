#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================================================"
echo "          FalconMindSDK 示例04: Bus消息总线 - 一键构建脚本"
echo "================================================================================"

for platform in x86 rk3588 rk3576 rv1126b; do
    echo "[${platform}] 构建中..."
    cd "$EXAMPLE_DIR/$platform"
    mkdir -p build && cd build
    if [ "$platform" = "x86" ]; then
        cmake -DCMAKE_BUILD_TYPE=Release ../.. > /dev/null 2>&1
    else
        cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../.. > /dev/null 2>&1
    fi
    make -j$(nproc) > /dev/null 2>&1
    echo "    ${platform} 构建成功"
done

echo ""
echo "================================================================================"
echo "                    所有平台构建成功"
echo "================================================================================"
