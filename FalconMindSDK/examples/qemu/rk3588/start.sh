#!/bin/bash
#
# RK3588 QEMU Startup Script
# 启动RK3588 ARM64虚拟机 (8核, 8GB内存)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_DIR="${SCRIPT_DIR}/images"
IMAGE_NAME="ubuntu-22.04-arm64-rk3588"
DISK_SIZE="64G"
MEMORY="8192"
CPUS="8"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_qemu() {
    if ! command -v qemu-system-aarch64 &> /dev/null; then
        print_error "qemu-system-aarch64 未安装"
        exit 1
    fi
}

start_vm() {
    local image_path="${IMAGE_DIR}/${IMAGE_NAME}.qcow2"
    mkdir -p "$IMAGE_DIR"

    if [[ ! -f "$image_path" ]]; then
        print_info "首次运行，准备镜像..."
        wget -O "${IMAGE_DIR}/jammy-server-cloudimg-arm64.img" \
            "https://cloud-images.ubuntu.com/releases/22.04/release/jammy-server-cloudimg-arm64.img"
        qemu-img convert -f qcow2 -O qcow2 "${IMAGE_DIR}/jammy-server-cloudimg-arm64.img" "$image_path"
        qemu-img resize "$image_path" "$DISK_SIZE"
    fi

    print_info "启动RK3588 QEMU虚拟机..."
    print_info "内存: ${MEMORY}MB, CPU: ${CPUS}核"
    print_info "SSH: localhost:2223"
    echo ""

    exec qemu-system-aarch64 \
        -m "$MEMORY" \
        -smp "$CPUS" \
        -cpu cortex-a76 \
        -M virt \
        -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
        -drive file="$image_path",if=virtio,format=qcow2 \
        -netdev user,id=net0,hostfwd=tcp::2223-:22 \
        -device virtio-net-device,netdev=net0 \
        -virtfs local,path="${SCRIPT_DIR}/../../..",mount_tag=host_share,security_model=mapped \
        -nographic
}

main() {
    echo "========================================"
    echo "  RK3588 QEMU ARM64 Virtual Machine"
    echo "========================================"
    echo ""
    check_qemu
    start_vm
}

main "$@"
