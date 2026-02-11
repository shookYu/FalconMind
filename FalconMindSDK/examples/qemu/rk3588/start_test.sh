#!/bin/bash
#
# RK3588 QEMU Test Script
# 启动ARM64虚拟机并运行测试
#

IMAGE_DIR="${SCRIPT_DIR:-./images}"
IMAGE_NAME="ubuntu-22.04-arm64.qcow2"
MEMORY="8192"
CPUS="8"
SSH_PORT="2223"

# 检查镜像
if [[ ! -f "${IMAGE_DIR}/${IMAGE_NAME}" ]]; then
    echo "错误: 镜像不存在: ${IMAGE_DIR}/${IMAGE_NAME}"
    exit 1
fi

echo "========================================"
echo "  RK3588 QEMU ARM64 Virtual Machine"
echo "========================================"
echo ""
echo "镜像: ${IMAGE_DIR}/${IMAGE_NAME}"
echo "内存: ${MEMORY}MB"
echo "CPU: ${CPUS}核"
echo "SSH: localhost:${SSH_PORT}"
echo ""
echo "按 Ctrl+A, X 退出虚拟机"
echo ""

# 启动QEMU
exec qemu-system-aarch64 \
    -m "$MEMORY" \
    -smp "$CPUS" \
    -cpu cortex-a76 \
    -M virt \
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
    -drive file="${IMAGE_DIR}/${IMAGE_NAME}",if=virtio,format=qcow2 \
    -netdev user,id=net0,hostfwd=tcp::${SSH_PORT}-:22 \
    -device virtio-net-device,netdev=net0 \
    -virtfs local,path="${SCRIPT_DIR}/../../..",mount_tag=host_share,security_model=mapped \
    -nographic
