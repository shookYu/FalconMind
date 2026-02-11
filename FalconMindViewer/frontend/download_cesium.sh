#!/bin/bash
# 下载 Cesium 到本地

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

CESIUM_VERSION="1.116.0"
# 使用 npm registry 的 tarball 链接（更可靠）
CESIUM_NPM_URL="https://registry.npmjs.org/cesium/-/cesium-${CESIUM_VERSION}.tgz"
LIB_DIR="libs/cesium"

echo "=========================================="
echo "下载 Cesium ${CESIUM_VERSION} 到本地"
echo "=========================================="
echo ""

# 创建目录
mkdir -p "$LIB_DIR"
cd libs

# 检查是否已存在
if [ -d "cesium/Build" ]; then
    echo "Cesium 已存在，跳过下载"
    exit 0
fi

# 下载
echo "正在从 npm registry 下载 Cesium..."
DOWNLOAD_SUCCESS=false

if command -v wget &> /dev/null; then
    if wget "$CESIUM_NPM_URL" -O cesium.tgz 2>/dev/null; then
        echo "正在解压..."
        mkdir -p cesium_temp
        tar -xzf cesium.tgz -C cesium_temp 2>/dev/null
        
        # npm 包的结构是 package/Build
        if [ -d "cesium_temp/package/Build" ]; then
            mv cesium_temp/package/Build cesium/
            rm -rf cesium_temp cesium.tgz
            DOWNLOAD_SUCCESS=true
        else
            echo "警告: 未找到 Build 目录，检查包结构..."
            ls -la cesium_temp/package/ | head -10
            rm -rf cesium_temp cesium.tgz
        fi
    fi
elif command -v curl &> /dev/null; then
    if curl -L "$CESIUM_NPM_URL" -o cesium.tgz 2>/dev/null; then
        echo "正在解压..."
        mkdir -p cesium_temp
        tar -xzf cesium.tgz -C cesium_temp 2>/dev/null
        
        # npm 包的结构是 package/Build
        if [ -d "cesium_temp/package/Build" ]; then
            mv cesium_temp/package/Build cesium/
            rm -rf cesium_temp cesium.tgz
            DOWNLOAD_SUCCESS=true
        else
            echo "警告: 未找到 Build 目录，检查包结构..."
            ls -la cesium_temp/package/ | head -10
            rm -rf cesium_temp cesium.tgz
        fi
    fi
else
    echo "错误: 未找到 wget 或 curl"
    exit 1
fi

if [ "$DOWNLOAD_SUCCESS" = false ]; then
    echo ""
    echo "❌ 自动下载失败。请使用以下方法之一："
    echo ""
    echo "方法 1: 使用 npm (推荐)"
    echo "  cd $SCRIPT_DIR"
    echo "  npm install cesium@${CESIUM_VERSION}"
    echo "  cp -r node_modules/cesium/Build libs/cesium/"
    echo ""
    echo "方法 2: 手动下载"
    echo "  访问: https://registry.npmjs.org/cesium/-/cesium-${CESIUM_VERSION}.tgz"
    echo "  下载后解压，将 package/Build 目录复制到 libs/cesium/"
    echo ""
    exit 1
fi

echo ""
echo "✅ Cesium 下载完成！"
echo "位置: $SCRIPT_DIR/$LIB_DIR"
echo ""
