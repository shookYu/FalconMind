#!/bin/bash
#
# NodeAgent 部署脚本
# 支持解耦架构（运行时加载 SDK）
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印函数
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助
show_help() {
    cat << EOF
NodeAgent 部署脚本

用法: $0 [选项] [命令]

命令:
    build           编译 NodeAgent（解耦模式）
    build-sdk       编译 SDK 共享库
    build-all       编译 SDK 和 NodeAgent
    deploy          部署到运行环境
    run             运行 NodeAgent
    check           检查部署环境
    clean           清理编译结果

选项:
    -h, --help      显示帮助
    -v, --verbose   显示详细输出
    --with-sdk      指定 SDK 路径（默认: ../build）

示例:
    $0 build-all                    # 编译全部
    $0 deploy                       # 部署
    $0 run                          # 运行
    $0 check                        # 检查环境
    $0 run --with-sdk /opt/sdk/lib  # 指定 SDK 路径

EOF
}

# 编译 SDK
build_sdk() {
    log_info "Building FalconMindSDK (shared library)..."
    
    cd "${PROJECT_ROOT}/FalconMindSDK"
    
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_SHARED_LIBS=ON \
             -DFALCONMINDSDK_BUILD_RKNN_BACKEND=ON
    
    make -j$(nproc)
    
    if [ -f "libfalconmind_sdk.so" ]; then
        log_info "SDK built successfully: $(pwd)/libfalconmind_sdk.so"
    else
        log_error "SDK library not found!"
        exit 1
    fi
}

# 编译 NodeAgent
build_nodeagent() {
    log_info "Building NodeAgent (decoupled mode)..."
    
    cd "${PROJECT_ROOT}/FalconMindSDK/NodeAgent"
    
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DNODEAGENT_STANDALONE=ON \
             -DNODEAGENT_USE_MQTT=OFF
    
    make -j$(nproc)
    
    if [ -f "nodeagent_demo" ]; then
        log_info "NodeAgent built successfully: $(pwd)/nodeagent_demo"
    else
        log_error "NodeAgent executable not found!"
        exit 1
    fi
    
    # 检查是否链接 SDK
    if ldd nodeagent_demo | grep -q falconmind; then
        log_warn "NodeAgent is linked with SDK library (should not happen in decoupled mode)"
    else
        log_info "✓ NodeAgent is NOT linked with SDK (decoupled mode verified)"
    fi
}

# 部署
deploy() {
    log_info "Deploying NodeAgent..."
    
    local deploy_dir="${1:-${PROJECT_ROOT}/deploy}"
    
    log_info "Deploy directory: ${deploy_dir}"
    
    # 创建部署目录结构
    mkdir -p "${deploy_dir}/bin"
    mkdir -p "${deploy_dir}/lib"
    mkdir -p "${deploy_dir}/config"
    mkdir -p "${deploy_dir}/plugins"
    mkdir -p "${deploy_dir}/logs"
    
    # 复制 SDK 库
    if [ -f "${PROJECT_ROOT}/FalconMindSDK/build/libfalconmind_sdk.so" ]; then
        cp "${PROJECT_ROOT}/FalconMindSDK/build/libfalconmind_sdk.so" "${deploy_dir}/lib/"
        log_info "✓ Copied SDK library"
    else
        log_error "SDK library not found! Run: $0 build-sdk"
        exit 1
    fi
    
    # 复制 NodeAgent
    if [ -f "${PROJECT_ROOT}/FalconMindSDK/NodeAgent/build/nodeagent_demo" ]; then
        cp "${PROJECT_ROOT}/FalconMindSDK/NodeAgent/build/nodeagent_demo" "${deploy_dir}/bin/"
        log_info "✓ Copied NodeAgent executable"
    else
        log_error "NodeAgent not found! Run: $0 build"
        exit 1
    fi
    
    # 复制配置文件模板
    cat > "${deploy_dir}/config/nodeagent.yaml" << 'EOF'
# NodeAgent 配置文件

uav:
  id: "UAV_001"
  name: "Alpha"

sdk:
  library_path: "../lib/libfalconmind_sdk.so"
  plugin_dir: "../plugins"

gcs:
  host: "192.168.1.100"
  port: 8080
  heartbeat_timeout_ms: 10000
  protocol: "mqtt"

mqtt:
  broker_host: "192.168.1.100"
  broker_port: 1883
  client_id: "nodeagent_uav001"
  topic_prefix: "uav"

autonomy:
  enabled: true
  max_offline_duration_minutes: 30
  low_battery_threshold: 30
  critical_battery_threshold: 15

storage:
  db_path: "../logs/offline.db"

logging:
  level: "INFO"
  format: "json"

metrics:
  enabled: true
  prometheus_port: 9090
EOF
    
    log_info "✓ Created configuration template"
    
    # 创建启动脚本
    cat > "${deploy_dir}/bin/start.sh" << 'EOF'
#!/bin/bash
# NodeAgent 启动脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 设置库路径
export LD_LIBRARY_PATH="${DEPLOY_DIR}/lib:${LD_LIBRARY_PATH}"

# 检查 SDK 库
if [ ! -f "${DEPLOY_DIR}/lib/libfalconmind_sdk.so" ]; then
    echo "Error: SDK library not found!"
    exit 1
fi

# 启动 NodeAgent
echo "Starting NodeAgent..."
echo "SDK Library: ${DEPLOY_DIR}/lib/libfalconmind_sdk.so"
echo ""

"${DEPLOY_DIR}/bin/nodeagent_demo" "$@"
EOF
    chmod +x "${deploy_dir}/bin/start.sh"
    
    log_info "✓ Created start script"
    
    log_info ""
    log_info "Deployment complete!"
    log_info ""
    log_info "To start NodeAgent:"
    log_info "  cd ${deploy_dir}"
    log_info "  ./bin/start.sh"
    log_info ""
}

# 检查环境
check_env() {
    log_info "Checking deployment environment..."
    
    local errors=0
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        log_error "g++ not found"
        errors=$((errors + 1))
    else
        log_info "✓ g++ found: $(g++ --version | head -1)"
    fi
    
    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        log_error "cmake not found"
        errors=$((errors + 1))
    else
        log_info "✓ cmake found: $(cmake --version | head -1)"
    fi
    
    # 检查依赖库
    if ! ldconfig -p | grep -q libsqlite3; then
        log_warn "libsqlite3 not found in system"
    else
        log_info "✓ libsqlite3 found"
    fi
    
    # 检查 SDK 源码
    if [ -d "${PROJECT_ROOT}/FalconMindSDK/include" ]; then
        log_info "✓ SDK source found"
    else
        log_error "SDK source not found"
        errors=$((errors + 1))
    fi
    
    # 检查 NodeAgent 源码
    if [ -d "${PROJECT_ROOT}/FalconMindSDK/NodeAgent/include" ]; then
        log_info "✓ NodeAgent source found"
    else
        log_error "NodeAgent source not found"
        errors=$((errors + 1))
    fi
    
    if [ $errors -eq 0 ]; then
        log_info ""
        log_info "✓ Environment check passed!"
        return 0
    else
        log_error ""
        log_error "✗ Environment check failed with ${errors} errors"
        return 1
    fi
}

# 运行 NodeAgent
run_nodeagent() {
    local sdk_path="${1:-${PROJECT_ROOT}/FalconMindSDK/build}"
    
    log_info "Running NodeAgent..."
    log_info "SDK Path: ${sdk_path}"
    
    cd "${PROJECT_ROOT}/FalconMindSDK/NodeAgent/build"
    
    if [ ! -f "nodeagent_demo" ]; then
        log_error "nodeagent_demo not found! Run: $0 build"
        exit 1
    fi
    
    export LD_LIBRARY_PATH="${sdk_path}:${LD_LIBRARY_PATH}"
    
    log_info ""
    ./nodeagent_demo
}

# 清理
clean() {
    log_info "Cleaning build artifacts..."
    
    rm -rf "${PROJECT_ROOT}/FalconMindSDK/build"
    rm -rf "${PROJECT_ROOT}/FalconMindSDK/NodeAgent/build"
    rm -rf "${PROJECT_ROOT}/deploy"
    
    log_info "✓ Clean complete"
}

# 主函数
main() {
    local command=""
    local sdk_path=""
    local verbose=0
    
    # 解析参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                verbose=1
                shift
                ;;
            --with-sdk)
                sdk_path="$2"
                shift 2
                ;;
            build|build-sdk|build-all|deploy|run|check|clean)
                command="$1"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    if [ -z "$command" ]; then
        show_help
        exit 1
    fi
    
    # 执行命令
    case $command in
        build-sdk)
            build_sdk
            ;;
        build)
            build_nodeagent
            ;;
        build-all)
            build_sdk
            build_nodeagent
            ;;
        deploy)
            deploy
            ;;
        run)
            run_nodeagent "$sdk_path"
            ;;
        check)
            check_env
            ;;
        clean)
            clean
            ;;
        *)
            log_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
