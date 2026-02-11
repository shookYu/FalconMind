#!/bin/bash
#
# FalconMindSDK + NodeAgent Build Script
# SDK和NodeAgent统一构建
#
# 用法:
#   ./build-all.sh <platform> [options]
#
# 平台:
#   x86      - 本地x86_64编译
#   arm64    - ARM64交叉编译
#
# 示例:
#   ./build-all.sh x86              # 编译x86版本
#   ./build-all.sh arm64           # 交叉编译arm64版本
#   ./build-all.sh arm64 install   # 编译并安装
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="${SCRIPT_DIR}/FalconMindSDK"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

BUILD_DIR="build"
INSTALL_PREFIX="install"
DO_INSTALL="false"
DO_CLEAN="false"
PLATFORM=""

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            x86|arm64)
                PLATFORM="$1"
                shift
                ;;
            install)
                DO_INSTALL="true"
                shift
                ;;
            clean)
                DO_CLEAN="true"
                shift
                ;;
            *)
                print_error "未知参数: $1"
                exit 1
                ;;
        esac
    done
}

check_dependencies() {
    print_info "检查构建依赖..."
    if ! command -v cmake &> /dev/null; then
        print_error "cmake 未安装"
        exit 1
    fi
    if ! command -v make &> /dev/null; then
        print_error "make 未安装"
        exit 1
    fi
    if [[ "$PLATFORM" == "arm64" ]]; then
        if [[ ! -d "/opt/FriendlyARM/toolchain/11.3-aarch64" ]]; then
            print_error "ARM64工具链未找到: /opt/FriendlyARM/toolchain/11.3-aarch64"
            exit 1
        fi
    fi
    print_success "依赖检查完成"
}

clean_build() {
    print_info "清理构建目录..."
    rm -rf "${SDK_DIR}/${BUILD_DIR}"
    print_success "清理完成"
}

build() {
    if [[ "$DO_CLEAN" == "true" ]]; then
        clean_build
        exit 0
    fi

    print_info "========== 构建 FalconMindSDK + NodeAgent ($PLATFORM) =========="

    mkdir -p "${SDK_DIR}/${BUILD_DIR}"
    cd "${SDK_DIR}/${BUILD_DIR}"

    if [[ "$PLATFORM" == "arm64" ]]; then
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE="${SDK_DIR}/toolchain/aarch64-linux-gnu.cmake" \
            -DFALCONMINDSDK_INSTALL_PREFIX="${BUILD_DIR}/${INSTALL_PREFIX}/arm64" \
            > /dev/null 2>&1
    else
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DFALCONMINDSDK_INSTALL_PREFIX="${BUILD_DIR}/${INSTALL_PREFIX}/x86" \
            > /dev/null 2>&1
    fi

    make -j$(nproc)
    print_success "SDK + NodeAgent 构建完成"

    if [[ "$DO_INSTALL" == "true" ]]; then
        make install
        print_success "安装完成"
    fi
}

show_usage() {
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  FalconMindSDK + NodeAgent 构建脚本${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}用法:${NC}"
    echo "  $0 <platform> [install] [clean]"
    echo ""
    echo -e "${YELLOW}平台:${NC}"
    echo "  x86    - 本地x86_64编译"
    echo "  arm64  - ARM64交叉编译"
    echo ""
    echo -e "${YELLOW}选项:${NC}"
    echo "  install  - 构建后安装"
    echo "  clean    - 清理后退出"
    echo ""
    echo -e "${YELLOW}示例:${NC}"
    echo "  $0 x86              # 构建x86版本"
    echo "  $0 arm64           # 交叉编译arm64版本"
    echo "  $0 x86 install     # 构建并安装"
    echo "  $0 arm64 install   # 交叉编译并安装"
    echo ""
}

main() {
    parse_args "$@"

    if [[ -z "$PLATFORM" ]]; then
        show_usage
        exit 1
    fi

    echo "========================================"
    echo "  FalconMindSDK + NodeAgent Build"
    echo "========================================"
    echo ""
    echo -e "${YELLOW}平台:${NC} $PLATFORM"
    echo -e "${YELLOW}SDK目录:${NC} $SDK_DIR"
    echo ""

    check_dependencies
    build
    print_success "构建完成!"
}

main "$@"
