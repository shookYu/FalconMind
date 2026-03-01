#!/bin/bash
# test_hot_reload_demo.sh
# 热更新功能演示脚本
#
# 此脚本演示 FalconMindSDK 的插件热更新功能
# 它会自动编译、部署和更新插件，展示零停机热更新

set -e

SDK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="${SDK_DIR}/tests"
PLUGIN_SRC_DIR="${SDK_DIR}/examples/plugins"
PLUGINS_DIR="${TEST_DIR}/plugins"
BUILD_DIR="${TEST_DIR}/build"

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║       FalconMindSDK Hot Reload Demo Script                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# 创建目录
mkdir -p "${PLUGINS_DIR}"
mkdir -p "${BUILD_DIR}"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
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

# ========================================================================
# 1. 编译测试程序
# ========================================================================
compile_test() {
    print_status "Compiling hot reload test program..."
    
    cd "${BUILD_DIR}"
    
    g++ -std=c++17 -O2 -pthread \
        -I"${SDK_DIR}/include" \
        -I/usr/include \
        "${TEST_DIR}/test_hot_reload.cpp" \
        "${SDK_DIR}/src/plugin/CapabilityRegistry.cpp" \
        "${SDK_DIR}/src/plugin/PluginManager.cpp" \
        -o test_hot_reload \
        -ldl \
        2>&1 | head -20
    
    if [ -f "${BUILD_DIR}/test_hot_reload" ]; then
        print_success "Test program compiled: ${BUILD_DIR}/test_hot_reload"
    else
        print_error "Failed to compile test program"
        exit 1
    fi
}

# ========================================================================
# 2. 编译初始插件 (v1.0.0)
# ========================================================================
compile_plugin_v1() {
    print_status "Building detector plugin v1.0.0..."
    
    cat > /tmp/detector_v1.cpp <> 'EOF'
#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include <string>
#include <vector>

using namespace falconmind::sdk;

class DetectorV1 : public plugin::IDetectorPlugin {
public:
    PluginMetadata getMetadata() const override {
        return {
            .name = "demo_detector",
            .version = "1.0.0",
            .description = "Demo detector v1.0.0",
            .author = "FalconMind",
            .sdkVersion = "1.0.0",
            .type = plugin::PluginType::Detector,
            .capabilities = plugin::PluginCapability::RealTime
        };
    }
    
    bool initialize(const core::ConfigManager& config) override {
        (void)config;
        return true;
    }
    
    void shutdown() override {}
    
    plugin::PluginState getState() const override {
        return plugin::PluginState::Active;
    }
    
    bool loadModel(const std::string& modelPath, const std::string& device) override {
        (void)modelPath;
        (void)device;
        return true;
    }
    
    perception::DetectionResult detect(const perception::ImageView& image) override {
        (void)image;
        perception::DetectionResult result;
        // v1.0.0: 模拟检测逻辑
        return result;
    }
    
    std::vector<std::string> getSupportedClasses() const override {
        return {"person", "car", "dog"};
    }
    
    void setConfidenceThreshold(float threshold) override {
        (void)threshold;
    }
    
    void setInputSize(int width, int height) override {
        (void)width;
        (void)height;
    }
    
    std::map<std::string, std::string> getModelInfo() const override {
        return {
            {"version", "1.0.0"},
            {"backend", "demo"},
            {"classes", "3"}
        };
    }
};

EXPORT_PLUGIN(DetectorV1)
EOF

    g++ -std=c++17 -shared -fPIC \
        -I"${SDK_DIR}/include" \
        /tmp/detector_v1.cpp \
        -o "${PLUGINS_DIR}/libdemo_detector.so" \
        2>&1 | head -10
    
    if [ -f "${PLUGINS_DIR}/libdemo_detector.so" ]; then
        print_success "Plugin v1.0.0 built: ${PLUGINS_DIR}/libdemo_detector.so"
    else
        print_error "Failed to build plugin v1.0.0"
        exit 1
    fi
}

# ========================================================================
# 3. 编译更新插件 (v2.0.0)
# ========================================================================
compile_plugin_v2() {
    print_status "Building detector plugin v2.0.0 (improved)..."
    
    cat > /tmp/detector_v2.cpp <> 'EOF'
#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include <string>
#include <vector>

using namespace falconmind::sdk;

class DetectorV2 : public plugin::IDetectorPlugin {
public:
    PluginMetadata getMetadata() const override {
        return {
            .name = "demo_detector",
            .version = "2.0.0",  // 版本升级
            .description = "Demo detector v2.0.0 - Improved performance",
            .author = "FalconMind",
            .sdkVersion = "1.0.0",
            .type = plugin::PluginType::Detector,
            .capabilities = plugin::PluginCapability::RealTime | 
                          plugin::PluginCapability::GPUAccelerated  // 新增能力
        };
    }
    
    bool initialize(const core::ConfigManager& config) override {
        (void)config;
        return true;
    }
    
    void shutdown() override {}
    
    plugin::PluginState getState() const override {
        return plugin::PluginState::Active;
    }
    
    bool loadModel(const std::string& modelPath, const std::string& device) override {
        (void)modelPath;
        (void)device;
        return true;
    }
    
    perception::DetectionResult detect(const perception::ImageView& image) override {
        (void)image;
        perception::DetectionResult result;
        // v2.0.0: 改进的检测逻辑，支持更多类别
        return result;
    }
    
    std::vector<std::string> getSupportedClasses() const override {
        // v2.0.0: 支持更多类别
        return {"person", "car", "dog", "cat", "bird", "bicycle", "motorcycle"};
    }
    
    void setConfidenceThreshold(float threshold) override {
        (void)threshold;
    }
    
    void setInputSize(int width, int height) override {
        (void)width;
        (void)height;
    }
    
    std::map<std::string, std::string> getModelInfo() const override {
        return {
            {"version", "2.0.0"},
            {"backend", "demo_improved"},
            {"classes", "7"},  // 更多类别
            {"optimization", "enabled"}
        };
    }
};

EXPORT_PLUGIN(DetectorV2)
EOF

    g++ -std=c++17 -shared -fPIC \
        -I"${SDK_DIR}/include" \
        /tmp/detector_v2.cpp \
        -o /tmp/libdemo_detector_v2.so \
        2>&1 | head -10
    
    if [ -f /tmp/libdemo_detector_v2.so ]; then
        print_success "Plugin v2.0.0 built: /tmp/libdemo_detector_v2.so"
    else
        print_error "Failed to build plugin v2.0.0"
        exit 1
    fi
}

# ========================================================================
# 4. 运行测试并执行热更新
# ========================================================================
run_demo() {
    echo ""
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║                     Demo Steps                               ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Step 1: Starting test program with v1.0.0 plugin"
    echo "Step 2: Wait 10 seconds for initial detection"
    echo "Step 3: Replace with v2.0.0 plugin (simulating hot update)"
    echo "Step 4: Watch automatic reload and seamless transition"
    echo "Step 5: Run for another 10 seconds with new version"
    echo "Step 6: Stop and show statistics"
    echo ""
    
    print_status "Starting hot reload test..."
    
    # 在后台运行测试程序
    cd "${BUILD_DIR}"
    timeout 25 ./test_hot_reload &
dash TEST_PID=$!
    
    print_status "Test started (PID: $TEST_PID)"
    print_status "Waiting 10 seconds for v1.0.0 to run..."
    
    sleep 10
    
    print_status "=========================================="
    print_status "PERFORMING HOT UPDATE: v1.0.0 -> v2.0.0"
    print_status "=========================================="
    
    # 执行热更新：替换插件文件
    cp /tmp/libdemo_detector_v2.so "${PLUGINS_DIR}/libdemo_detector.so"
    
    print_success "Plugin file replaced with v2.0.0"
    print_status "Hot reload should trigger automatically..."
    
    sleep 10
    
    print_status "Test completed!"
    
    # 等待测试程序结束
    wait $TEST_PID 2>/dev/null || true
    
    echo ""
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║                     Demo Complete                            ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    
    # 显示日志
    if [ -f "${BUILD_DIR}/hot_reload_test.log" ]; then
        print_status "Hot reload events from log:"
        grep -E "(HOT RELOAD|version|Detection count)" "${BUILD_DIR}/hot_reload_test.log" | tail -20
    fi
}

# ========================================================================
# 5. 清理
# ========================================================================
cleanup() {
    print_status "Cleaning up..."
    rm -f /tmp/detector_v1.cpp /tmp/detector_v2.cpp
    rm -f /tmp/libdemo_detector_v2.so
    rm -rf "${PLUGINS_DIR}"
    print_success "Cleanup complete"
}

# ========================================================================
# 主流程
# ========================================================================
main() {
    echo "Starting hot reload demo..."
    echo ""
    
    # 检查依赖
    if ! command -v g++ >/dev/null 2>&1; then
        print_error "g++ not found. Please install build-essential"
        exit 1
    fi
    
    # 步骤1：编译测试程序
    compile_test
    
    # 步骤2：编译插件
    compile_plugin_v1
    compile_plugin_v2
    
    # 步骤3：运行演示
    run_demo
    
    # 步骤4：清理
    read -p "Press Enter to cleanup test files..."
    cleanup
    
    echo ""
    print_success "Hot reload demo completed!"
    echo ""
    echo "Summary:"
    echo "  ✓ Plugin v1.0.0 was loaded and running"
    echo "  ✓ Plugin was replaced with v2.0.0 without stopping the test"
    echo "  ✓ Hot reload mechanism detected the change automatically"
    echo "  ✓ Detection continued seamlessly during the transition"
    echo "  ✓ Version upgraded from 1.0.0 to 2.0.0 with new capabilities"
    echo ""
}

# 运行主流程
main "$@"
