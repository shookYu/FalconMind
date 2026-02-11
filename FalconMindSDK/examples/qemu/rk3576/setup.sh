#!/bin/bash
#
# RK3576 QEMU Environment Setup Script
# 在虚拟机中运行此脚本设置编译环境
#

set -e

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

# 配置
EXAMPLES_DIR="${HOME}/examples"
SDK_DIR="${HOME}/sdk"
TOOLCHAIN_DIR="/opt/FriendlyARM/toolchain/11.3-aarch64"
TOOLCHAIN_URL="https://github.com/friendlyarm/prebuilts.git"

# 检查是否在QEMU中运行
check_qemu() {
    if [[ $(uname -m) != "aarch64" ]]; then
        print_warning "当前不在ARM64环境中，此脚本应在QEMU虚拟机内运行"
        print_info "用法: ./start.sh 启动QEMU后，在虚拟机中运行此脚本"
    fi
}

# 检查并安装依赖
install_dependencies() {
    print_info "安装编译依赖..."

    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        gcc \
        g++ \
        libssl-dev \
        wget \
        git \
        vim \
        net-tools \
        curl \
        ca-certificates \
        pkg-config \
        libglib2.0-dev \
        libpixman-1-dev

    print_success "依赖安装完成"
}

# 安装交叉编译工具链
install_toolchain() {
    if [[ -d "$TOOLCHAIN_DIR" ]]; then
        print_info "工具链已安装: $TOOLCHAIN_DIR"
        ls "$TOOLCHAIN_DIR/bin/" | head -5
        return 0
    fi

    print_info "安装ARM64交叉编译工具链..."

    sudo mkdir -p /opt/FriendlyARM
    cd /tmp

    # 下载FriendlyARM工具链
    if [[ -d "prebuilts" ]]; then
        rm -rf prebuilts
    fi

    git clone --depth 1 "$TOOLCHAIN_URL" prebuilts
    sudo cp -r prebuilts/gcc-x86_64 /opt/FriendlyARM/toolchain/11.3-aarch64

    print_success "工具链安装完成"
    print_info "工具链路径: $TOOLCHAIN_DIR/bin/"
}

# 配置环境变量
setup_env() {
    print_info "配置环境变量..."

    # 添加到 .bashrc
    local bashrc_content='
# FalconMindSDK Environment
export ARM_TOOLCHAIN_DIR=/opt/FriendlyARM/toolchain/11.3-aarch64
export PATH=$ARM_TOOLCHAIN_DIR/bin:$PATH
export CROSS_COMPILE=$ARM_TOOLCHAIN_DIR/bin/aarch64-linux-gnu-
export FALCONMIND_SDK_DIR=$HOME/sdk
export PKG_CONFIG_PATH=$ARM_TOOLCHAIN_DIR/aarch64-cortexa53-linux-gnu/sysroot/usr/lib/pkgconfig:$PKG_CONFIG_PATH
'

    if ! grep -q "FalconMindSDK Environment" ~/.bashrc 2>/dev/null; then
        echo "$bashrc_content" >> ~/.bashrc
        print_info "已添加到 ~/.bashrc"
    fi

    # 当前会话应用
    export ARM_TOOLCHAIN_DIR=/opt/FriendlyARM/toolchain/11.3-aarch64
    export PATH=$ARM_TOOLCHAIN_DIR/bin:$PATH

    print_success "环境变量配置完成"
}

# 挂载共享目录
mount_shared() {
    print_info "检查共享目录..."

    if [[ -d "/host_share" ]]; then
        print_info "共享目录已挂载"
        return 0
    fi

    sudo mkdir -p /host_share
    sudo mount -t 9p -o trans=virtio,version=9p2000.L host_share /host_share || {
        print_warning "无法挂载共享目录，请手动设置"
    }
}

# 设置examples
setup_examples() {
    print_info "设置examples..."

    mkdir -p "$EXAMPLES_DIR"

    # 链接共享目录
    if [[ -d "/host_share/examples" ]]; then
        ln -sf /host_share/examples "$EXAMPLES_DIR" 2>/dev/null || true
        print_info "Examples链接到: $EXAMPLES_DIR"
    fi

    # 链接SDK
    if [[ -d "/host_share" ]]; then
        ln -sf /host_share "$SDK_DIR" 2>/dev/null || true
        print_info "SDK链接到: $SDK_DIR"
    fi
}

# 安装SDK依赖
install_sdk_deps() {
    print_info "安装SDK运行时依赖..."

    sudo apt-get install -y \
        libglib2.0-0 \
        libncurses5 \
        libstdc++6

    # 检查是否需要特定库
    print_info "检查可选依赖..."
    sudo apt-get install -y \
        libopencv-dev \
        libv4l-dev \
        || print_warning "部分可选依赖安装失败，继续..."
}

# 编译examples
build_examples() {
    if [[ ! -d "$EXAMPLES_DIR" ]]; then
        print_error "Examples目录不存在: $EXAMPLES_DIR"
        return 1
    fi

    print_info "编译examples..."

    cd "$EXAMPLES_DIR"

    # 编译RK3576平台
    if [[ -d "rk3576" ]]; then
        print_info "编译RK3576 examples..."
        bash build.sh rk3576
    else
        print_warning "RK3576目录不存在，跳过"
    fi
}

# 运行测试
run_tests() {
    if [[ ! -d "$EXAMPLES_DIR/rk3576/build" ]]; then
        print_error "编译产物不存在，请先运行 build.sh"
        return 1
    fi

    print_info "运行测试..."

    cd "$EXAMPLES_DIR"
    bash run-tests.sh rk3576
}

# 显示使用说明
show_usage() {
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  RK3576 环境设置完成!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}常用命令:${NC}"
    echo "  ./setup.sh              # 运行完整设置"
    echo "  ./setup.sh --deps      # 仅安装依赖"
    echo "  ./setup.sh --toolchain # 仅安装工具链"
    echo "  ./setup.sh --build     # 编译examples"
    echo "  ./setup.sh --test      # 运行测试"
    echo ""
    echo -e "${YELLOW}工具链路径:${NC} $TOOLCHAIN_DIR"
    echo -e "${YELLOW}Examples目录:${NC} $EXAMPLES_DIR"
    echo -e "${YELLOW}编译命令:${NC} cd $EXAMPLES_DIR && bash build.sh rk3576"
    echo -e "${YELLOW}测试命令:${NC} cd $EXAMPLES_DIR && bash run-tests.sh rk3576"
    echo ""
}

# 主程序
main() {
    local ACTION="all"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --deps)
                ACTION="deps"
                shift
                ;;
            --toolchain)
                ACTION="toolchain"
                shift
                ;;
            --build)
                ACTION="build"
                shift
                ;;
            --test)
                ACTION="test"
                shift
                ;;
            --all)
                ACTION="all"
                shift
                ;;
            *)
                print_error "未知参数: $1"
                exit 1
                ;;
        esac
    done

    echo "========================================"
    echo "  RK3576 QEMU Environment Setup"
    echo "========================================"
    echo ""

    case "$ACTION" in
        deps)
            install_dependencies
            install_sdk_deps
            ;;
        toolchain)
            install_toolchain
            setup_env
            ;;
        build)
            check_qemu
            build_examples
            ;;
        test)
            check_qemu
            run_tests
            ;;
        all)
            check_qemu
            install_dependencies
            install_sdk_deps
            install_toolchain
            setup_env
            mount_shared
            setup_examples
            build_examples
            run_tests
            show_usage
            ;;
    esac

    print_success "完成!"
}

main "$@"
