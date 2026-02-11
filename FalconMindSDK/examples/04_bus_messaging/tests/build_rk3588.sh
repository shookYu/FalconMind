#!/bin/bash
set -e
SCRIPT_DIR="/home/shook/study/opencode"
EXAMPLE_DIR="."

echo "构建 rk3588 平台..."
cd "/rk3588"
mkdir -p build && cd build
if [ "rk3588" = "x86" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release ../..
else
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../..
fi
make -j22
echo "完成"
