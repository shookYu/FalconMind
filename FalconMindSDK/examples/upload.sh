#!/bin/bash
#
# FalconMindSDK Examples Upload Script
# =====================================
# 上传examples到目标平台进行编译和测试
#
# 用法:
#   ./upload.sh <platform> [--build] [--run] [--qemu]
#
# 支持的平台:
#   x86      - 本地x86_64 Ubuntu 22.04
#   rk3576   - RK3576 开发板 / QEMU虚拟机
#   rk3588   - RK3588 开发板 / QEMU虚拟机
#   rv1126b  - RV1126B 开发板 / QEMU虚拟机
#
# 示例:
#   ./upload.sh rk3576 --build --run        # 上传到RK3576板卡并编译运行
#   ./upload.sh rk3576 --qemu              # 启动RK3576 QEMU虚拟机
#   ./upload.sh rk3588 --qemu --build      # 启动QEMU并编译
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
EXAMPLES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$(dirname "$EXAMPLES_DIR")"
UPLOAD_DIR="$EXAMPLES_DIR/.upload_cache"

# 目标平台配置
declare -A PLATFORM_CONFIG
PLATFORM_CONFIG=(
    ["x86"]="x86_64-unknown-linux-gnu"
    ["rk3576"]="aarch64-unknown-linux-gnu"
    ["rk3588"]="aarch64-unknown-linux-gnu"
    ["rv1126b"]="aarch64-unknown-linux-gnu"
)

# 工具链路径
declare -A TOOLCHAIN_PATHS
TOOLCHAIN_PATHS=(
    ["rk3576"]="/opt/FriendlyARM/toolchain/11.3-aarch64"
    ["rk3588"]="/opt/FriendlyARM/toolchain/11.3-aarch64"
    ["rv1126b"]="/opt/FriendlyARM/toolchain/11.3-aarch64"
)

# 板卡IP地址 (默认配置，需要用户修改)
declare -A BOARD_IPS
BOARD_IPS=(
    ["rk3576"]="192.168.1.100"
    ["rk3588"]="192.168.1.101"
    ["rv1126b"]="192.168.1.102"
)

# 板卡用户名
declare -A BOARD_USERS
BOARD_USERS=(
    ["rk3576"]="root"
    ["rk3588"]="root"
    ["rv1126b"]="root"
)

# 板卡密码
declare -A BOARD_PASSWORDS
BOARD_PASSWORDS=(
    ["rk3576"]="password"
    ["rk3588"]="password"
    ["rv1126b"]="password"
)

# 板卡工作目录
declare -A BOARD_PATHS
BOARD_PATHS=(
    ["rk3576"]="/root/falconmind_examples"
    ["rk3588"]="/root/falconmind_examples"
    ["rv1126b"]="/root/falconmind_examples"
)

# QEMU配置
declare -A QEMU_CONFIGS
QEMU_CONFIGS=(
    ["x86"]="qemu/x86"
    ["rk3576"]="qemu/rk3576"
    ["rk3588"]="qemu/rk3588"
    ["rv1126b"]="qemu/rv1126b"
)

# QEMU内存
declare -A QEMU_MEMORY
QEMU_MEMORY=(
    ["x86"]="4096"
    ["rk3576"]="4096"
    ["rk3588"]="8192"
    ["rv1126b"]="2048"
)

# QEMU CPU核心数
declare -A QEMU_CPUS
QEMU_CPUS=(
    ["x86"]="4"
    ["rk3576"]="4"
    ["rk3588"]="6"
    ["rv1126b"]="2"
)

# QEMU磁盘大小
declare -A QEMU_DISK
QEMU_DISK=(
    ["x86"]="64"
    ["rk3576"]="64"
    ["rk3588"]="64"
    ["rv1126b"]="32"
)

# 函数：打印帮助信息
print_help() {
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}  FalconMindSDK Examples Upload Tool${NC}"
    echo -e "${BLUE}======================================${NC}"
    echo ""
    echo -e "${YELLOW}用法:${NC}"
    echo "  $0 <platform> [options]"
    echo ""
    echo -e "${YELLOW}支持的平台:${NC}"
    echo "  x86      - x86_64 Ubuntu 22.04 (本地/QEMU)"
    echo "  rk3576  - RK3576 开发板 / QEMU虚拟机"
    echo "  rk3588  - RK3588 开发板 / QEMU虚拟机"
    echo "  rv1126b - RV1126B 开发板 / QEMU虚拟机"
    echo ""
    echo -e "${YELLOW}选项:${NC}"
    echo "  --build    - 编译examples"
    echo "  --run      - 运行examples"
    echo "  --qemu     - 启动QEMU虚拟机"
    echo "  --sync     - 同步文件到板卡"
    echo "  --all      - 执行所有操作(build+run)"
    echo "  --help     - 显示帮助信息"
    echo ""
    echo -e "${YELLOW}示例:${NC}"
    echo "  $0 rk3576 --build --run    # 上传并编译运行"
    echo "  $0 rk3576 --qemu          # 启动QEMU"
    echo "  $0 rk3588 --sync          # 同步文件到板卡"
    echo "  $0 x86 --all              # 本地完整测试"
    echo ""
    echo -e "${YELLOW}配置文件:${NC}"
    echo "  板卡IP和密码配置在: $0 顶部的 BOARD_IPS/BOARD_PASSWORDS"
    echo ""
}

# 函数：打印信息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 函数：检查平台参数
check_platform() {
    local platform="$1"
    if [[ ! "${PLATFORM_CONFIG[$platform]}" ]]; then
        print_error "不支持的平台: $platform"
        echo "支持的平台: x86 rk3576 rk3588 rv1126b"
        exit 1
    fi
}

# 函数：检查依赖工具
check_dependencies() {
    print_info "检查依赖工具..."

    local missing_tools=""

    # 基础工具
    for tool in scp ssh rsync; do
        if ! command -v $tool &> /dev/null; then
            missing_tools="$missing_tools $tool"
        fi
    done

    if [[ -n "$missing_tools" ]]; then
        print_warning "缺少工具:$missing_tools"
        print_info "安装: apt-get install openssh-client rsync"
    fi

    # QEMU相关
    if [[ "$USE_QEMU" == "true" ]]; then
        if ! command -v qemu-system-aarch64 &> /dev/null; then
            print_warning "qemu-system-aarch64 未安装"
        fi
        if ! command -v virt-install &> /dev/null; then
            print_warning "virt-install 未安装 (可选)"
        fi
    fi

    print_success "依赖检查完成"
}

# 函数：创建上传目录
prepare_upload_dir() {
    local platform="$1"
    local upload_path="$UPLOAD_DIR/$platform"

    print_info "准备上传目录: $upload_path"
    mkdir -p "$upload_path"

    # 复制SDK头文件
    if [[ -d "$SDK_DIR/install/${platform}/include" ]]; then
        cp -r "$SDK_DIR/install/${platform}/include" "$upload_path/"
        print_info "已复制SDK头文件"
    fi

    # 复制SDK库
    if [[ -f "$SDK_DIR/install/${platform}/lib/libfalconmind_sdk.a" ]]; then
        mkdir -p "$upload_path/lib"
        cp "$SDK_DIR/install/${platform}/lib/libfalconmind_sdk.a" "$upload_path/lib/"
        print_info "已复制SDK库"
    fi

    # 复制工具链
    local toolchain_path="${TOOLCHAIN_PATHS[$platform]}"
    if [[ -d "$toolchain_path" && "$platform" != "x86" ]]; then
        mkdir -p "$upload_path/toolchain"
        cp -r "$toolchain_path"/* "$upload_path/toolchain/" 2>/dev/null || true
        print_info "已复制工具链"
    fi

    echo "$upload_path"
}

# 函数：复制examples到上传目录
copy_examples() {
    local platform="$1"
    local upload_path="$2"

    print_info "复制examples到上传目录..."
    mkdir -p "$upload_path/examples"

    # 获取所有example目录
    for example_dir in "$EXAMPLES_DIR"/[0-9]*; do
        if [[ -d "$example_dir" ]]; then
            local example_name=$(basename "$example_dir")
            
            # 检查该平台是否存在
            if [[ -d "$example_dir/$platform" ]]; then
                cp -r "$example_dir/$platform" "$upload_path/examples/$example_name"
                print_info "  复制: $example_name ($platform)"
            fi
        fi
    done

    # 复制公共脚本
    if [[ -f "$EXAMPLES_DIR/build.sh" ]]; then
        cp "$EXAMPLES_DIR/build.sh" "$upload_path/"
    fi
}

# 函数：同步到板卡
sync_to_board() {
    local platform="$1"
    local upload_path="$2"

    local board_ip="${BOARD_IPS[$platform]}"
    local board_user="${BOARD_USERS[$platform]}"
    local board_password="${BOARD_PASSWORDS[$platform]}"
    local board_path="${BOARD_PATHS[$platform]}"

    print_info "同步到板卡: $board_user@$board_ip:$board_path"

    # 创建远程目录
    sshpass -p "$board_password" ssh -o StrictHostKeyChecking=no \
        "$board_user@$board_ip" "mkdir -p $board_path"

    # 同步文件
    sshpass -p "$board_password" rsync -avz --progress \
        -e "ssh -o StrictHostKeyChecking=no" \
        "$upload_path/" \
        "$board_user@$board_ip:$board_path/"

    print_success "同步完成: $board_ip:$board_path"
}

# 函数：在板卡上编译
build_on_board() {
    local platform="$1"

    local board_ip="${BOARD_IPS[$platform]}"
    local board_user="${BOARD_USERS[$platform]}"
    local board_password="${BOARD_PASSWORDS[$platform]}"
    local board_path="${BOARD_PATHS[$platform]}"

    print_info "在板卡上编译..."

    # 编译所有examples
    sshpass -p "$board_password" ssh -o StrictHostKeyChecking=no \
        "$board_user@$board_ip" "cd $board_path && bash build.sh $platform"

    print_success "编译完成"
}

# 函数：在板卡上运行测试
run_on_board() {
    local platform="$1"
    local example="${2:-all}"

    local board_ip="${BOARD_IPS[$platform]}"
    local board_user="${BOARD_USERS[$platform]}"
    local board_password="${BOARD_PASSWORDS[$platform]}"
    local board_path="${BOARD_PATHS[$platform]}"

    print_info "在板卡上运行测试..."

    sshpass -p "$board_password" ssh -o StrictHostKeyChecking=no \
        "$board_user@$board_ip" "cd $board_path && bash run-tests.sh $platform $example"
}

# 函数：启动QEMU虚拟机
start_qemu() {
    local platform="$1"
    local qemu_config="${QEMU_CONFIGS[$platform]}"
    local qemu_dir="$EXAMPLES_DIR/$qemu_config"

    print_info "启动QEMU虚拟机 ($platform)..."

    if [[ -f "$qemu_dir/start.sh" ]]; then
        chmod +x "$qemu_dir/start.sh"
        bash "$qemu_dir/start.sh"
    else
        print_error "QEMU配置不存在: $qemu_dir"
        print_info "请先创建QEMU配置: examples/$qemu_config/"
        exit 1
    fi
}

# 函数：执行QEMU内操作
exec_in_qemu() {
    local platform="$1"
    local cmd="$2"
    local qemu_config="${QEMU_CONFIGS[$platform]}"
    local qemu_dir="$EXAMPLES_DIR/$qemu_config"

    if [[ -f "$qemu_dir/exec.sh" ]]; then
        bash "$qemu_dir/exec.sh" "$cmd"
    else
        print_error "QEMU exec脚本不存在"
    fi
}

# 函数：完整流程 - 板卡
full_deploy_board() {
    local platform="$1"

    print_info "========== 开始部署到 $platform 板卡 =========="

    # 1. 准备上传目录
    local upload_path=$(prepare_upload_dir "$platform")

    # 2. 复制examples
    copy_examples "$platform" "$upload_path"

    # 3. 同步到板卡
    sync_to_board "$platform" "$upload_path"

    # 4. 编译
    if [[ "$DO_BUILD" == "true" ]]; then
        build_on_board "$platform"
    fi

    # 5. 运行
    if [[ "$DO_RUN" == "true" ]]; then
        run_on_board "$platform"
    fi

    print_success "========== $platform 部署完成 =========="
}

# 函数：完整流程 - QEMU
full_deploy_qemu() {
    local platform="$1"
    local qemu_config="${QEMU_CONFIGS[$platform]}"
    local qemu_dir="$EXAMPLES_DIR/$qemu_config"

    print_info "========== 开始QEMU测试 $platform =========="

    # 1. 启动QEMU
    print_info "启动QEMU虚拟机..."
    bash "$qemu_dir/start.sh" &

    # 2. 等待虚拟机启动
    print_info "等待虚拟机启动..."
    sleep 10

    # 3. 同步文件到虚拟机
    print_info "同步文件到虚拟机..."
    local vm_ip="127.0.0.1"
    local vm_port="${QEMU_SSH_PORT:-2222}"
    local vm_user="falcon"
    local vm_password="falcon"

    # 4. 编译
    if [[ "$DO_BUILD" == "true" ]]; then
        print_info "在QEMU中编译..."
        bash "$qemu_dir/exec.sh" "cd /home/falcon/examples && bash build.sh $platform"
    fi

    # 5. 运行
    if [[ "$DO_RUN" == "true" ]]; then
        print_info "在QEMU中运行测试..."
        bash "$qemu_dir/exec.sh" "cd /home/falcon/examples && bash run-tests.sh $platform"
    fi

    print_success "========== QEMU测试完成 =========="
}

# 函数：显示使用统计
show_statistics() {
    local platform="$1"

    echo ""
    echo -e "${BLUE}========== $platform 平台统计 ==========${NC}"

    local example_count=0
    local compiled_count=0

    for example_dir in "$EXAMPLES_DIR"/[0-9]*; do
        if [[ -d "$example_dir" ]]; then
            if [[ -d "$example_dir/$platform" ]]; then
                ((example_count++))
                if [[ -f "$example_dir/$platform/build/01_"* ]]; then
                    ((compiled_count++))
                fi
            fi
        fi
    done

    echo "  Examples数量: $example_count"
    echo "  已编译: $compiled_count"
    echo "  待编译: $((example_count - compiled_count))"
}

# 主程序
main() {
    local platform=""
    local DO_BUILD="false"
    local DO_RUN="false"
    local USE_QEMU="false"
    local DO_SYNC="false"
    local DO_ALL="false"

    # 解析参数
    while [[ $# -gt 0 ]]; do
        case "$1" in
            x86|rk3576|rk3588|rv1126b)
                platform="$1"
                shift
                ;;
            --build)
                DO_BUILD="true"
                shift
                ;;
            --run)
                DO_RUN="true"
                shift
                ;;
            --qemu)
                USE_QEMU="true"
                shift
                ;;
            --sync)
                DO_SYNC="true"
                shift
                ;;
            --all)
                DO_BUILD="true"
                DO_RUN="true"
                DO_ALL="true"
                shift
                ;;
            --help|-h)
                print_help
                exit 0
                ;;
            *)
                print_error "未知参数: $1"
                print_help
                exit 1
                ;;
        esac
    done

    # 检查平台参数
    if [[ -z "$platform" ]]; then
        print_error "请指定平台"
        print_help
        exit 1
    fi

    check_platform "$platform"

    # 检查依赖
    check_dependencies

    # 创建上传缓存目录
    mkdir -p "$UPLOAD_DIR"

    # 显示配置信息
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}  FalconMindSDK Examples Upload${NC}"
    echo -e "${BLUE}======================================${NC}"
    echo ""
    echo -e "${YELLOW}平台:${NC} $platform"
    echo -e "${YELLOW}架构:${NC} ${PLATFORM_CONFIG[$platform]}"
    echo -e "${YELLOW}工具链:${NC} ${TOOLCHAIN_PATHS[$platform]:-本地}"
    if [[ "$USE_QEMU" == "true" ]]; then
        echo -e "${YELLOW}模式:${NC} QEMU虚拟机"
        echo -e "${YELLOW}内存:${NC} ${QEMU_MEMORY[$platform]}MB"
        echo -e "${YELLOW}CPU:${NC} ${QEMU_CPUS[$platform]}核"
    else
        echo -e "${YELLOW}模式:${NC} 物理板卡"
        echo -e "${YELLOW}IP地址:${NC} ${BOARD_IPS[$platform]}"
    fi
    echo ""

    # 根据参数执行
    if [[ "$USE_QEMU" == "true" ]]; then
        # QEMU模式
        if [[ "$DO_ALL" == "true" ]]; then
            full_deploy_qemu "$platform"
        else
            start_qemu "$platform"
        fi
    elif [[ "$DO_SYNC" == "true" || "$DO_BUILD" == "true" || "$DO_RUN" == "true" ]]; then
        # 板卡模式
        full_deploy_board "$platform"
    else
        # 仅显示统计
        show_statistics "$platform"
    fi

    print_success "完成!"
}

# 运行主程序
main "$@"
