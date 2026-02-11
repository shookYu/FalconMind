#!/bin/bash
set -e
SCRIPT_DIR="/home/shook/study/opencode"
EXAMPLE_DIR="."

echo "构建 rk3576 平台..."
cd "/rk3576"
mkdir -p build && cd build
if [ "rk3576" = "x86" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release ../..
else
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../..
fi
make -j22
echo "完成"
