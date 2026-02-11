#!/bin/bash
#
# FalconMindSDK QEMU Test Script
# 在QEMU虚拟机中编译和测试所有examples
#
# 用法:
#   ./run-qemu-test.sh <platform> [--build] [--run] [--all]
#
# 示例:
#   ./run-qemu-test.sh rk3576 --all    # 完整测试流程
#   ./run-qemu-test.sh rk3588 --build  # 仅编译
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 平台SSH配置
declare -A SSH_PORTS
SSH_PORTS=(
    ["x86"]="2222"
    ["rk3576"]="2222"
    ["rk3588"]="2223"
    ["rv1126b"]="2224"
)

# QEMU目录
declare -A QEMU_DIRS
QEMU_DIRS=(
    ["x86"]="qemu/x86"
    ["rk3576"]="qemu/rk3576"
    ["rk3588"]="qemu/rk3588"
    ["rv1126b"]="qemu/rv1126b"
)

# 等待虚拟机SSH就绪
wait_for_ssh() {
    local port="$1"
    local max_attempts=30
    local attempt=1

    print_info "等待SSH服务就绪 (端口: $port)..."

    while [[ $attempt -le $max_attempts ]]; do
        if sshpass -p "falcon" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=2 "falcon@localhost" -p "$port" "echo 'SSH OK'" 2>/dev/null; then
            print_success "SSH服务已就绪"
            return 0
        fi
        sleep 2
        ((attempt++))
    done

    print_error "SSH服务未能在规定时间内就绪"
    return 1
}

# 同步代码到虚拟机
sync_to_vm() {
    local platform="$1"
    local qemu_dir="${QEMU_DIRS[$platform]}"

    print_info "同步代码到虚拟机..."

    # 通过virtfs共享目录，代码已经在虚拟机中可用
    print_info "代码通过virtfs共享，无需额外同步"
}

# 在虚拟机中执行命令
exec_in_vm() {
    local platform="$1"
    local cmd="$2"
    local port="${SSH_PORTS[$platform]}"

    sshpass -p "falcon" ssh -o StrictHostKeyChecking=no "falcon@localhost" -p "$port" "$cmd"
}

# 安装依赖
install_deps_vm() {
    local platform="$1"
    local qemu_dir="${QEMU_DIRS[$platform]}"

    print_info "安装编译依赖..."
    exec_in_vm "$platform" "sudo apt-get update && sudo apt-get install -y build-essential cmake gcc g++ libssl-dev"
}

# 设置工具链
setup_toolchain_vm() {
    local platform="$1"

    if [[ "$platform" == "x86" ]]; then
        print_info "x86平台使用本地工具链"
        return 0
    fi

    print_info "配置ARM64交叉编译工具链..."
    exec_in_vm "$platform" "export ARM_TOOLCHAIN_DIR=/opt/FriendlyARM/toolchain/11.3-aarch64 && export PATH=\$ARM_TOOLCHAIN_DIR/bin:\$PATH && echo 'Toolchain configured'"
}

# 编译examples
build_vm() {
    local platform="$1"

    print_info "编译examples..."

    local build_cmd='
        cd ~/examples
        bash build.sh '"$platform"'
    '

    exec_in_vm "$platform" "$build_cmd"
}

# 运行测试
run_tests_vm() {
    local platform="$1"

    print_info "运行测试..."

    local run_cmd='
        cd ~/examples
        bash run-tests.sh '"$platform"'
    '

    exec_in_vm "$platform" "$run_cmd"
}

# 启动QEMU
start_qemu() {
    local platform="$1"
    local qemu_dir="${QEMU_DIRS[$platform]}"

    print_info "启动QEMU虚拟机..."
    bash "${SCRIPT_DIR}/${qemu_dir}/start.sh" &
    sleep 5
}

# 完整测试流程
full_test() {
    local platform="$1"

    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  FalconMindSDK QEMU测试: $platform${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # 1. 启动QEMU
    start_qemu "$platform"

    # 2. 等待SSH
    local port="${SSH_PORTS[$platform]}"
    wait_for_ssh "$port"

    # 3. 设置环境
    install_deps_vm "$platform"
    setup_toolchain_vm "$platform"

    # 4. 同步代码
    sync_to_vm "$platform"

    # 5. 编译
    build_vm "$platform"

    # 6. 运行测试
    run_tests_vm "$platform"
}

# 主程序
main() {
    local platform="${1:-}"
    local DO_BUILD="false"
    local DO_RUN="false"
    local DO_ALL="false"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --build)
                DO_BUILD="true"
                shift
                ;;
            --run)
                DO_RUN="true"
                shift
                ;;
            --all)
                DO_BUILD="true"
                DO_RUN="true"
                DO_ALL="true"
                shift
                ;;
            x86|rk3576|rk3588|rv1126b)
                platform="$1"
                shift
                ;;
            *)
                shift
                ;;
        esac
    done

    if [[ -z "$platform" ]]; then
        print_error "请指定平台: x86 rk3576 rk3588 rv1126b"
        exit 1
    fi

    if [[ "$DO_ALL" == "true" ]]; then
        full_test "$platform"
    elif [[ "$DO_BUILD" == "true" && "$DO_RUN" == "true" ]]; then
        local port="${SSH_PORTS[$platform]}"
        setup_toolchain_vm "$platform"
        build_vm "$platform"
        run_tests_vm "$platform"
    elif [[ "$DO_BUILD" == "true" ]]; then
        setup_toolchain_vm "$platform"
        build_vm "$platform"
    elif [[ "$DO_RUN" == "true" ]]; then
        run_tests_vm "$platform"
    else
        print_info "用法: $0 <platform> [--build] [--run] [--all]"
        print_info "示例: $0 rk3576 --all"
    fi

    print_success "完成!"
}

main "$@"
