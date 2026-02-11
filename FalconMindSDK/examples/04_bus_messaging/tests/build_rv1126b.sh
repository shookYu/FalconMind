#!/bin/bash
set -e
SCRIPT_DIR="/home/shook/study/opencode"
EXAMPLE_DIR="."

echo "构建 rv1126b 平台..."
cd "/rv1126b"
mkdir -p build && cd build
if [ "rv1126b" = "x86" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release ../..
else
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ../..
fi
make -j22
echo "完成"
