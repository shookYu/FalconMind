#!/bin/bash
#
# RK3576 QEMU Startup Script
# 启动RK3576 ARM64虚拟机
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VM_DIR="${SCRIPT_DIR}"
IMAGE_DIR="${SCRIPT_DIR}/images"
IMAGE_NAME="ubuntu-22.04-arm64-rk3576"
IMAGE_URL="https://cdimage.ubuntu.com/releases/22.04/release/ubuntu-22.04.3-live-server-arm64.iso"
DISK_SIZE="64G"
MEMORY="8192"
CPUS="8"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查QEMU
check_qemu() {
    print_info "检查QEMU安装..."
    if ! command -v qemu-system-aarch64 &> /dev/null; then
        print_error "qemu-system-aarch64 未安装"
        print_info "安装: sudo apt-get install qemu-system-aarch64"
        exit 1
    fi
    print_info "QEMU版本: $(qemu-system-aarch64 --version | head -1)"
}

# 下载Ubuntu镜像
download_image() {
    mkdir -p "$IMAGE_DIR"
    local image_path="${IMAGE_DIR}/${IMAGE_NAME}.qcow2"

    if [[ -f "$image_path" ]]; then
        print_info "磁盘镜像已存在: $image_path"
        echo "$image_path"
        return 0
    fi

    print_info "下载Ubuntu 22.04 ARM64镜像..."
    wget -O "${IMAGE_DIR}/ubuntu-22.04-arm64.iso" "$IMAGE_URL" || {
        print_warning "下载失败，尝试使用cloud-image"
        download_cloud_image
        return $?
    }

    # 创建磁盘
    print_info "创建磁盘镜像..."
    qemu-img create -f qcow2 "$image_path" "$DISK_SIZE"

    # 启动安装程序
    print_info "启动安装程序..."
    qemu-system-aarch64 \
        -m "$MEMORY" \
        -smp "$CPUS" \
        -cpu cortex-a57 \
        -M virt \
        -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
        -cdrom "${IMAGE_DIR}/ubuntu-22.04-arm64.iso" \
        -drive file="$image_path",if=virtio,format=qcow2 \
        -netdev user,id=net0,hostfwd=tcp::2222-:22 \
        -device virtio-net-device,netdev=net0 \
        -nographic

    print_warning "安装完成后，请重新运行此脚本启动虚拟机"
}

# 使用cloud-image快速启动
download_cloud_image() {
    print_info "使用Ubuntu cloud-image..."
    local cloud_image="jammy-server-cloudimg-arm64.img"
    local image_path="${IMAGE_DIR}/${IMAGE_NAME}.qcow2"

    wget -O "${IMAGE_DIR}/${cloud_image}" \
        "https://cloud-images.ubuntu.com/releases/22.04/release/${cloud_image}"

    # 转换并扩展磁盘
    qemu-img convert -f qcow2 -O qcow2 "${IMAGE_DIR}/${cloud_image}" "$image_path"
    qemu-img resize "$image_path" "$DISK_SIZE"

    # 创建cloud-init配置
    create_cloud_init

    print_info "Cloud-image已准备就绪"
}

# 创建cloud-init配置
create_cloud_init() {
    local ci_dir="${IMAGE_DIR}/cloud-init"
    mkdir -p "$ci_dir"

    # User data
    cat > "${ci_dir}/user-data" << 'EOF'
#cloud-config
hostname: rk3576-vm
username: falcon
password: falcon
chpasswd: { expire: False }
ssh_pwauth: True
disable_root: false
groups:
  - sudo
users:
  - default
package_update: true
packages:
  - build-essential
  - cmake
  - gcc-aarch64-linux-gnu
  - g++-aarch64-linux-gnu
  - libssl-dev
  - wget
  - git
  - vim
  - net-tools
runcmd:
  - echo "falcon ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
  - systemctl enable ssh
EOF

    # Meta data
    cat > "${ci_dir}/meta-data" << 'EOF'
instance-id: rk3576-vm
local-hostname: rk3576-vm
EOF

    # Network config
    cat > "${ci_dir}/network-config" << 'EOF'
version: 2
ethernets:
  eth0:
    dhcp4: true
EOF
}

# 启动虚拟机
start_vm() {
    local image_path="${IMAGE_DIR}/${IMAGE_NAME}.qcow2"

    if [[ ! -f "$image_path" ]]; then
        print_info "首次运行，正在准备镜像..."
        download_cloud_image
    fi

    print_info "启动RK3576 QEMU虚拟机..."
    print_info "内存: ${MEMORY}MB, CPU: ${CPUS}核"
    print_info "SSH: localhost:2222"
    print_info "用户: falcon, 密码: falcon"
    print_info "按 Ctrl+A, X 退出"
    echo ""

    # 启动QEMU
    exec qemu-system-aarch64 \
        -m "$MEMORY" \
        -smp "$CPUS" \
        -cpu cortex-a57 \
        -M virt \
        -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
        -drive file="$image_path",if=virtio,format=qcow2 \
        -netdev user,id=net0,hostfwd=tcp::2222-:22 \
        -device virtio-net-device,netdev=net0 \
        -virtfs local,path="${VM_DIR}/../../..,mount_tag=host_share,security_model=mapped" \
        -nographic \
        -serial mon:stdio
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."

    if ! command -v wget &> /dev/null; then
        print_warning "wget 未安装"
    fi

    if [[ ! -f /usr/share/qemu-efi-aarch64/QEMU_EFI.fd ]]; then
        print_warning "QEMU EFI固件未找到，尝试安装..."
        sudo apt-get install -y qemu-efi-aarch64 2>/dev/null || true
    fi
}

# 主程序
main() {
    echo "========================================"
    echo "  RK3576 QEMU ARM64 Virtual Machine"
    echo "========================================"
    echo ""

    check_dependencies
    check_qemu
    start_vm
}

main "$@"
