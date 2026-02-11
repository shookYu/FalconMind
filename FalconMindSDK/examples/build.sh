#!/bin/bash
#
# FalconMindSDK Examples Build Script
# 编译所有examples或指定平台
#
# 用法:
#   ./build.sh <platform> [example_number]
#
# 示例:
#   ./build.sh x86              # 编译所有x86 examples
#   ./build.sh rk3576 01       # 仅编译01 example
#   ./build.sh rk3588          # 编译所有RK3588 examples
#

set -e

EXAMPLES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="$(dirname "$EXAMPLES_DIR")"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 平台配置
declare -A PLATFORM_CMAKE
PLATFORM_CMAKE=(
    ["x86"]="x86"
    ["rk3576"]="rk3576"
    ["rk3588"]="rk3588"
    ["rv1126b"]="rv1126b"
)

# 编译单个example
build_example() {
    local platform="$1"
    local example_num="$2"

    local example_dir="${EXAMPLES_DIR}/${example_num}"
    local platform_dir="${example_dir}/${platform}"
    local build_dir="${platform_dir}/build"

    if [[ ! -d "$platform_dir" ]]; then
        print_warning "平台目录不存在: $platform_dir"
        return 1
    fi

    print_info "编译 ${example_num} ($platform)..."

    # 创建build目录
    mkdir -p "$build_dir"

    # CMake配置
    cd "$build_dir"
    cmake "$platform_dir" > /dev/null 2>&1 || {
        print_error "CMake配置失败: $platform_dir"
        return 1
    }

    # 编译
    make -j$(nproc) || {
        print_error "编译失败: $example_num/$platform"
        return 1
    }

    print_success "编译成功: $example_num/$platform"
}

# 编译所有examples
build_all() {
    local platform="$1"

    print_info "========== 开始编译 $platform 平台所有examples =========="

    local success=0
    local failed=0

    for example_dir in "${EXAMPLES_DIR}"/[0-9]*; do
        if [[ -d "$example_dir" ]]; then
            local example_num=$(basename "$example_dir")

            if build_example "$platform" "$example_num"; then
                ((success++))
            else
                ((failed++))
            fi
        fi
    done

    echo ""
    print_info "========== 编译完成 =========="
    print_info "成功: $success"
    [[ $failed -gt 0 ]] && print_warning "失败: $failed"
}

# 检查平台参数
check_platform() {
    local platform="$1"

    if [[ -z "$platform" ]]; then
        print_error "请指定平台: x86 rk3576 rk3588 rv1126b"
        exit 1
    fi

    if [[ ! "${PLATFORM_CMAKE[$platform]}" ]]; then
        print_error "不支持的平台: $platform"
        exit 1
    fi
}

# 主程序
main() {
    local platform="${1:-}"
    local example="${2:-}"

    check_platform "$platform"

    echo "========================================"
    echo "  FalconMindSDK Examples Build"
    echo "========================================"
    echo ""
    print_info "平台: $platform"
    print_info "SDK目录: $SDK_DIR"
    echo ""

    if [[ -n "$example" ]]; then
        build_example "$platform" "$example"
    else
        build_all "$platform"
    fi

    print_success "编译完成!"
}

main "$@"
