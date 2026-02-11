#!/bin/bash
#
# x86 QEMU Virtual Machine (用于完整系统测试)
# 本地x86_64 Ubuntu 22.04
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_DIR="${SCRIPT_DIR}/images"
IMAGE_NAME="ubuntu-22.04-x86_64"
DISK_SIZE="64G"
MEMORY="4096"
CPUS="4"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[INFO]${NC} $1"; }

check_qemu() {
    if ! command -v qemu-system-x86_64 &> /dev/null; then
        print_info "使用VirtualBox或本地系统"
        return 1
    fi
}

start_vm() {
    local image_path="${IMAGE_DIR}/${IMAGE_NAME}.qcow2"
    mkdir -p "$IMAGE_DIR"

    if [[ ! -f "$image_path" ]]; then
        print_info "下载Ubuntu 22.04镜像..."
        wget -O "${IMAGE_DIR}/jammy-server-cloudimg-amd64.img" \
            "https://cloud-images.ubuntu.com/releases/22.04/release/jammy-server-cloudimg-amd64.img"
        qemu-img convert -f qcow2 -O qcow2 "${IMAGE_DIR}/jammy-server-cloudimg-amd64.img" "$image_path"
        qemu-img resize "$image_path" "$DISK_SIZE"
    fi

    print_info "启动x86 QEMU虚拟机..."
    exec qemu-system-x86_64 \
        -m "$MEMORY" \
        -smp "$CPUS" \
        -drive file="$image_path",if=virtio,format=qcow2 \
        -netdev user,id=net0,hostfwd=tcp::2222-:22 \
        -device virtio-net-device,netdev=net0 \
        -nographic
}

main() {
    echo "========================================"
    echo "  x86 QEMU Virtual Machine"
    echo "========================================"
    check_qemu || {
        echo "QEMU x86未安装，直接在本地编译即可"
        echo "cd examples && bash build.sh x86"
        exit 0
    }
    start_vm
}

main "$@"
