#!/bin/bash
#
# RV1126B QEMU Environment Setup Script
#

set -e

EXAMPLES_DIR="${HOME}/examples"
TOOLCHAIN_DIR="/opt/FriendlyARM/toolchain/11.3-aarch64"

print_info() { echo -e "\033[0;32m[INFO]\033[0m $1"; }

install_deps() {
    print_info "安装依赖..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential cmake gcc g++ \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        libssl-dev wget git vim net-tools
}

install_toolchain() {
    if [[ -d "$TOOLCHAIN_DIR" ]]; then
        print_info "工具链已安装"
        return 0
    fi
    print_info "安装工具链..."
    sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
}

main() {
    echo "========================================"
    echo "  RV1126B QEMU Environment Setup"
    echo "========================================"
    install_deps
    install_toolchain
    print_info "完成!"
}

main "$@"
