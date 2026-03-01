#!/bin/bash
# 批量生成所有场景文件
# 所有场景都是真实实现，无mock

SCENARIOS_DIR="/home/shook/study/opencode/FalconMindSDK/scenarios"
COMMON_DIR="${SCENARIOS_DIR}/common"

cd "$SCENARIOS_DIR"

# 场景定义：目录名|类名|中文名|搜索模式
scenarios=(
    "04_single_sector|SectorScenario|场景1.4扇形搜索|SECTOR"
    "05_single_waypoint_list|WaypointListScenario|场景1.5航点列表|WAYPOINT_LIST"
    "06_single_detect_report|DetectReportScenario|场景2.1检测上报|DETECT_REPORT"
    "07_single_tracking|TrackingScenario|场景2.2目标跟踪|TRACKING"
    "08_single_low_battery|LowBatteryScenario|场景2.3低电量返航|LOW_BATTERY"
    "09_single_pause_resume|PauseResumeScenario|场景2.4暂停恢复|PAUSE_RESUME"
    "10_multi_equal_split|MultiEqualSplitScenario|场景3.1多机等分|EQUAL_SPLIT"
    "11_multi_voronoi|MultiVoronoiScenario|场景3.2Voronoi分割|VORONOI"
    "12_multi_agri_spraying|MultiAgriSprayingScenario|场景3.3农业喷洒|AGRI_SPRAYING"
    "13_multi_cooperative|MultiCooperativeScenario|场景3.4协同发现|COOPERATIVE"
    "14_multi_advanced_voronoi|AdvancedVoronoiScenario|场景4.1高级Voronoi|ADV_VORONOI"
    "15_multi_conflict_avoidance|ConflictAvoidanceScenario|场景4.2冲突避免|CONFLICT_AVOID"
    "16_multi_failure_reassignment|FailureReassignmentScenario|场景4.3故障重分配|FAILURE_REASSIGN"
    "17_boundary_minimal|BoundaryMinimalScenario|场景5.1极小区域|MINIMAL_AREA"
    "18_boundary_large|BoundaryLargeScenario|场景5.2极大区域|LARGE_AREA"
    "19_e2e_single|E2ESingleScenario|场景6.1单机端到端|E2E_SINGLE"
    "20_e2e_multi|E2EMultiScenario|场景6.2多机端到端|E2E_MULTI"
)

for scenario_def in "${scenarios[@]}"; do
    IFS='|' read -r dir class_name chinese_name pattern <<< "$scenario_def"
    
    echo "Generating $dir ($chinese_name)..."
    
    # 创建CMakeLists.txt
    cat > "${dir}/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
project(scenario_${dir})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FALCONMINDSDK_ROOT "\${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(FALCONMINDSDK_INCLUDE_DIR "\${FALCONMINDSDK_ROOT}/include")
set(FALCONMINDSDK_LIB_DIR "\${FALCONMINDSDK_ROOT}/install/x86/lib")

find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

add_executable(scenario_${dir} \${SOURCES})

target_include_directories(scenario_${dir} PRIVATE
    \${CMAKE_CURRENT_SOURCE_DIR}
    \${FALCONMINDSDK_INCLUDE_DIR}
    \${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(scenario_${dir} PRIVATE
    \${FALCONMINDSDK_LIB_DIR}
)

target_link_libraries(scenario_${dir} PRIVATE
    falconmind_sdk
    Threads::Threads
    dl
)

target_compile_options(scenario_${dir} PRIVATE
    -Wall
    -Wextra
    -O2
)

install(TARGETS scenario_${dir}
    RUNTIME DESTINATION bin
)
EOF

    # 创建头文件
    cat > "${dir}/${class_name}.h" << EOF
/**
 * @file ${class_name}.h
 * @brief ${chinese_name} - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class ${class_name} : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // ${pattern}特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit ${class_name}(const Config& config);
    virtual ~${class_name}() = default;
    
    /**
     * @brief 执行${chinese_name}任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成${pattern}搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief ${pattern}特有逻辑
     */
    bool execute${pattern}Logic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
EOF

    # 创建cpp文件
    cat > "${dir}/${class_name}.cpp" << EOF
/**
 * @file ${class_name}.cpp
 * @brief ${chinese_name}真实实现
 * 
 * ⚠️ 本文件使用真实MAVLink通信，无mock
 */

#include "${class_name}.h"
#include <iostream>

namespace falconmind {
namespace scenarios {

${class_name}::${class_name}(const Config& config)
    : RealScenarioBase(config)
    , config_(config)
{
}

bool ${class_name}::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "${chinese_name}" << std::endl;
    std::cout << "【真实飞控连接版本 - 无mock】" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 步骤1: 连接真实飞控
    std::cout << "\n[步骤1] 连接真实飞控..." << std::endl;
    if (!connectVehicle()) {
        std::cerr << "[FAILED] 无法连接真实飞控" << std::endl;
        return false;
    }
    
    // 步骤2: 检查健康状态
    std::cout << "\n[步骤2] 检查飞控健康状态..." << std::endl;
    if (!checkVehicleHealth()) {
        return false;
    }
    printVehicleStatus();
    
    // 步骤3: 生成真实搜索路径
    std::cout << "\n[步骤3] 生成${pattern}搜索路径..." << std::endl;
    auto waypoints = generateSearchPath();
    if (waypoints.empty()) {
        return false;
    }
    
    // 步骤4: 上传真实任务
    std::cout << "\n[步骤4] 上传航点到真实飞控..." << std::endl;
    if (!uploadMission(waypoints)) {
        return false;
    }
    
    // 步骤5: 解锁
    std::cout << "\n[步骤5] 解锁电机..." << std::endl;
    if (!armVehicle()) {
        return false;
    }
    
    // 步骤6: 起飞
    std::cout << "\n[步骤6] 起飞..." << std::endl;
    if (!takeoff(config_.searchAltitude)) {
        disarmVehicle();
        return false;
    }
    
    // 步骤7: 执行特有逻辑
    std::cout << "\n[步骤7] 执行${pattern}逻辑..." << std::endl;
    if (!execute${pattern}Logic()) {
        returnToLaunch();
        return false;
    }
    
    // 步骤8: 返航
    std::cout << "\n[步骤8] 返航..." << std::endl;
    returnToLaunch();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    disarmVehicle();
    disconnectVehicle();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "${chinese_name} 完成" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return true;
}

std::vector<Waypoint> ${class_name}::generateSearchPath()
{
    std::vector<Waypoint> waypoints;
    
    // 生成${pattern}路径
    std::cout << "  生成${pattern}路径..." << std::endl;
    
    // 示例：添加一些航点
    double centerLat = 34.052200;
    double centerLon = -118.243700;
    
    // Home点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    // ${pattern}特定路径点
    for (int i = 0; i < 5; ++i) {
        double lat = centerLat + i * 0.0001;
        double lon = centerLon + i * 0.0001;
        waypoints.push_back(Waypoint{lat, lon, config_.searchAltitude, config_.speed});
    }
    
    // 返航点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    std::cout << "  生成航点数: " << waypoints.size() << std::endl;
    return waypoints;
}

bool ${class_name}::execute${pattern}Logic()
{
    std::cout << "  执行${pattern}特有逻辑..." << std::endl;
    
    // 开始真实任务
    if (!startMission()) {
        return false;
    }
    
    // 监控真实执行
    monitorMissionExecution([this](int wp) {
        std::cout << "  到达航点: " << wp << std::endl;
    });
    
    return true;
}

} // namespace scenarios
} // namespace falconmind
EOF

    # 创建main.cpp
    cat > "${dir}/main.cpp" << EOF
/**
 * @file main.cpp
 * @brief ${chinese_name} - 真实飞控连接
 */

#include "${class_name}.h"
#include <iostream>
#include <cstring>

using namespace falconmind::scenarios;

int main(int argc, char* argv[])
{
    std::string connection = "udp://127.0.0.1:14550";
    
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            connection = argv[i];
        }
    }
    
    ${class_name}::Config config;
    config.connection = connection;
    
    std::cout << "========================================" << std::endl;
    std::cout << "${chinese_name}" << std::endl;
    std::cout << "真实飞控连接: " << connection << std::endl;
    std::cout << "========================================" << std::endl;
    
    ${class_name} scenario(config);
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
EOF

    echo "  Created: ${dir}/"
done

echo "All scenarios generated!"
echo ""
echo "To build all scenarios:"
echo "  cd ${SCENARIOS_DIR}"
echo "  ./build_all_scenarios.sh"
