#!/bin/bash
cd /home/shook/study/opencode/FalconMindSDK/examples/qemu/rk3588

# 停止之前的QEMU
pkill -9 qemu-system-aarch64 2>/dev/null
sleep 2

echo "=== 启动RK3588 QEMU虚拟机 ==="
echo "镜像: images/ubuntu-22.04-arm64.qcow2"
echo "SSH: localhost:2223 (用户: ubuntu, 密码: ubuntu)"
echo ""
echo "启动中，请等待约30秒..."

# 启动QEMU
exec qemu-system-aarch64 \
    -m 4096 \
    -smp 4 \
    -cpu cortex-a72 \
    -M virt \
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
    -drive file=images/ubuntu-22.04-arm64.qcow2,if=virtio,format=qcow2 \
    -netdev user,id=net0,hostfwd=tcp::2223-:22 \
    -device virtio-net-device,netdev=net0 \
    -virtfs local,path=/home/shook/study/opencode/FalconMindSDK/examples/qemu,mount_tag=host_share,security_model=mapped \
    -nographic
