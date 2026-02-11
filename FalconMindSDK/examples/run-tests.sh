#!/bin/bash
#
# FalconMindSDK Examples Test Runner
# 运行所有examples或指定example的测试
#
# 用法:
#   ./run-tests.sh <platform> [example_number]
#
# 示例:
#   ./run-tests.sh x86              # 运行所有x86 tests
#   ./run-tests.sh rk3576 01        # 仅运行01 test
#

set -e

EXAMPLES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================================================${NC}"
}

print_info() { echo -e "${GREEN}[TEST]${NC} $1"; }
print_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[FAIL]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 运行单个example
run_example() {
    local platform="$1"
    local example_num="$2"

    local example_dir="${EXAMPLES_DIR}/${example_num}"
    local platform_dir="${example_dir}/${platform}"
    local binary="${platform_dir}/build/01_${example_num}_${platform}"

    if [[ ! -f "$binary" ]]; then
        print_warning "二进制文件不存在: $binary"
        return 1
    fi

    print_header "Example ${example_num}: ${platform}"

    # 运行测试
    if "$binary"; then
        print_success "测试通过: $example_num/$platform"
        return 0
    else
        print_warning "测试失败: $example_num/$platform"
        return 1
    fi
}

# 运行所有examples
run_all() {
    local platform="$1"

    print_header "FalconMindSDK Tests: ${platform}"

    local passed=0
    local failed=0
    local total=0

    for example_dir in "${EXAMPLES_DIR}"/[0-9]*; do
        if [[ -d "$example_dir" ]]; then
            local example_num=$(basename "$example_dir")
            ((total++))

            if run_example "$platform" "$example_num"; then
                ((passed++))
            else
                ((failed++))
            fi
        fi
    done

    echo ""
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "${BLUE}  测试结果汇总${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    print_info "总数: $total"
    print_success "通过: $passed"
    [[ $failed -gt 0 ]] && print_warning "失败: $failed"

    [[ $failed -eq 0 ]]
}

# 检查平台参数
check_platform() {
    local platform="$1"

    if [[ -z "$platform" ]]; then
        print_error "请指定平台: x86 rk3576 rk3588 rv1126b"
        exit 1
    fi
}

# 主程序
main() {
    local platform="${1:-}"
    local example="${2:-}"

    check_platform "$platform"

    echo "========================================"
    echo "  FalconMindSDK Examples Test Runner"
    echo "========================================"

    if [[ -n "$example" ]]; then
        run_example "$platform" "$example"
    else
        run_all "$platform"
    fi
}

main "$@"
