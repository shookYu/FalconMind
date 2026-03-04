/**
 * @file main.cpp
 * @brief SDK Native Implementation - Denied Environment Visual Tracking
 * 
 * 拒止环境区域侦查与视觉制导跟踪 - C++原生实现
 * 
 * 编译：
 *   mkdir build && cd build
 *   cmake .. -DFALCONMINDSDK_BUILD_TESTS=ON
 *   make -j4
 * 
 * 运行：
 *   ./denied_env_tracking --config config.yaml
 */

#include <iostream>
#include <string>
#include <signal>
#include <atomic>
#include <math>

#include <denied_env_tracking/mission.hpp>
#include <denied_env_tracking/navigation.hpp>
#include <denied_env_tracking/perception.hpp>
#include <denied_env_tracking/control.hpp>

using namespace falconmind::denied_env_tracking;

// 全局退出标志
std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\n[Signal] Received signal " << signum << ", shutting down...\n";
    g_running = false;
}

/**
 * @brief 打印任务状态
 */
void printMissionState(const MissionState& state) {
    std::string phase_str;
    switch (state.phase) {
        case MissionPhase::INITIALIZING: phase_str = "INITIALIZING"; break;
        case MissionPhase::SEARCHING: phase_str = "SEARCHING"; break;
        case MissionPhase::TARGET_ACQUIRED: phase_str = "TARGET_ACQUIRED"; break;
        case MissionPhase::TRACKING: phase_str = "TRACKING"; break;
        case MissionPhase::RETURNING: phase_str = "RETURNING"; break;
        case MissionPhase::LANDED: phase_str = "LANDED"; break;
        case MissionPhase::ABORTED: phase_str = "ABORTED"; break;
    }
    
    std::cout << "\n[Mission State]\n";
    std::cout << "  Phase: " << phase_str << "\n";
    std::cout << "  GNSS Status: " << static_cast<int>(state.gnss_status) << "\n";
    std::cout << "  VINS Initialized: " << (state.vins_initialized ? "YES" : "NO") << "\n";
    std::cout << "  GPS Spoofing: " << (state.gps_spoofing_detected ? "DETECTED" : "OK") << "\n";
    std::cout << "  Targets Detected: " << state.detected_targets.size() << "\n";
    std::cout << "  Battery: " << state.battery_percent << "%\n";
    
    if (state.selected_target) {
        std::cout << "  Selected Target: " << state.selected_target->track_id 
                  << " (Confirmed: " << (state.selected_target->confirmed ? "YES" : "NO") << ")\n";
    }
    
    if (state.phase == MissionPhase::TRACKING) {
        std::cout << "  Tracking Duration: " << state.tracking_duration << "s\n";
        std::cout << "  Current Distance: " << state.current_distance << "m\n";
        std::cout << "  Current Height: " << state.current_height << "m\n";
    }
}

/**
 * @brief 打印使用说明
 */
void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "\nOptions:\n"
              << "  -h, --help              Show this help message\n"
              << "  -c, --config FILE       Configuration file (YAML)\n"
              << "  -m, --mission ID        Mission ID\n"
              << "  -u, --uav ID           UAV ID\n"
              << "  -d, --distance M       Desired tracking distance (default: 30m)\n"
              << "  -H, --height M         Desired tracking height (default: 10m)\n"
              << "  --no-gcs               Disable GCS communication\n"
              << "  --vins-only            Use VINS only (ignore GNSS)\n"
              << "\nInteractive Commands:\n"
              << "  s [id]                 Select target by ID\n"
              << "  c                      Confirm selected target\n"
              << "  a                      Abort mission\n"
              << "  r                      Return to launch\n"
              << "  p                      Print current state\n"
              << "  q                      Quit\n"
              << "\nExample:\n"
              << "  " << program << " -m mission_001 -u uav_001\n";
}

/**
 * @brief 解析命令行参数
 */
MissionConfig parseArgs(int argc, char** argv) {
    MissionConfig config;
    config.mission_id = "denied_env_001";
    config.uav_id = "uav_001";
    config.desired_distance = 30.0;
    config.desired_height = 10.0;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            // 从文件加载配置
            ++i;
        } else if ((arg == "-m" || arg == "--mission") && i + 1 < argc) {
            config.mission_id = argv[++i];
        } else if ((arg == "-u" || arg == "--uav") && i + 1 < argc) {
            config.uav_id = argv[++i];
        } else if ((arg == "-d" || arg == "--distance") && i + 1 < argc) {
            config.desired_distance = std::stod(argv[++i]);
        } else if ((arg == "-H" || arg == "--height") && i + 1 < argc) {
            config.desired_height = std::stod(argv[++i]);
        }
    }
    
    return config;
}

/**
 * @brief 交互式命令处理
 */
void handleInteractiveCommand(DeniedEnvTrackingMission& mission, 
                               const std::string& cmd) {
    if (cmd.empty()) return;
    
    char command = cmd[0];
    
    switch (command) {
        case 's': {
            // 选择目标
            int track_id = std::stoi(cmd.substr(2));
            if (mission.selectTarget(track_id, "operator_local")) {
                std::cout << "Selected target " << track_id << ", waiting for confirmation...\n";
            } else {
                std::cout << "Failed to select target " << track_id << "\n";
            }
            break;
        }
        case 'c':
            // 确认目标
            if (mission.confirmTarget(true, "operator_local")) {
                std::cout << "Target confirmed, starting tracking...\n";
            } else {
                std::cout << "Failed to confirm target\n";
            }
            break;
        case 'a':
            // 中止任务
            mission.abort("Operator abort");
            std::cout << "Mission aborted\n";
            break;
        case 'r':
            // 返航
            mission.handleGCSCommand("RETURN", "");
            std::cout << "Returning to launch...\n";
            break;
        case 'p': {
            // 打印状态
            auto state = mission.getState();
            printMissionState(state);
            break;
        }
        case 'q':
            // 退出
            g_running = false;
            break;
        default:
            std::cout << "Unknown command: " << cmd << "\n";
            break;
    }
}

int main(int argc, char** argv) {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 打印启动信息
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║     FalconMind SDK - Denied Environment Visual Tracking       ║
║                                                               ║
║  Mission: Search and track target in GPS denied environment   ║
║  Version: 1.0.0                                              ║
╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    // 解析配置
    MissionConfig config = parseArgs(argc, argv);
    
    std::cout << "\n[Configuration]\n";
    std::cout << "  Mission ID: " << config.mission_id << "\n";
    std::cout << "  UAV ID: " << config.uav_id << "\n";
    std::cout << "  Desired Distance: " << config.desired_distance << "m\n";
    std::cout << "  Desired Height: " << config.desired_height << "m\n";
    std::cout << "  Search Altitude: " << config.search_altitude << "m\n";
    std::cout << "  VINS Init Time: " << config.vins_init_time << "s\n";
    std::cout << "  GPS Defense: " << (config.enable_gps_defense ? "ENABLED" : "DISABLED") << "\n\n";
    
    // 设置回调
    MissionCallbacks callbacks;
    callbacks.on_phase_transition = [](MissionPhase from, MissionPhase to) {
        std::string from_str, to_str;
        switch (from) {
            case MissionPhase::INITIALIZING: from_str = "INITIALIZING"; break;
            case MissionPhase::SEARCHING: from_str = "SEARCHING"; break;
            case MissionPhase::TARGET_ACQUIRED: from_str = "TARGET_ACQUIRED"; break;
            case MissionPhase::TRACKING: from_str = "TRACKING"; break;
            default: from_str = "UNKNOWN"; break;
        }
        switch (to) {
            case MissionPhase::SEARCHING: to_str = "SEARCHING"; break;
            case MissionPhase::TARGET_ACQUIRED: to_str = "TARGET_ACQUIRED"; break;
            case MissionPhase::TRACKING: to_str = "TRACKING"; break;
            case MissionPhase::RETURNING: to_str = "RETURNING"; break;
            case MissionPhase::ABORTED: to_str = "ABORTED"; break;
            default: to_str = "UNKNOWN"; break;
        }
        std::cout << "[Phase Transition] " << from_str << " -> " << to_str << "\n";
    };
    
    callbacks.on_target_detected = [](const TargetDetection& target) {
        std::cout << "[Target Detected] ID: " << target.track_id 
                  << ", Class: " <> target.class_name
                  << ", Conf: " << target.confidence << "\n";
    };
    
    callbacks.on_target_selected = [](int track_id) {
        std::cout << "[Target Selected] ID: " << track_id << "\n";
    };
    
    callbacks.on_target_confirmed = []() {
        std::cout << "[Target Confirmed] Starting tracking...\n";
    };
    
    callbacks.on_tracking_update = [](double distance, double height) {
        // 每秒打印一次跟踪状态
        static auto last_print = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 1) {
            std::cout << "[Tracking] Distance: " << distance << "m, Height: " << height << "m\n";
            last_print = now;
        }
    };
    
    callbacks.on_error = [](const std::string& error) {
        std::cerr << "[ERROR] " << error << "\n";
    };
    
    callbacks.on_mission_complete = []() {
        std::cout << "\n[Mission Complete]\n";
    };
    
    try {
        // 创建任务
        auto mission = createDeniedEnvTrackingMission(config, callbacks);
        
        // 初始化
        std::cout << "Initializing mission...\n";
        if (!mission->initialize()) {
            std::cerr << "Failed to initialize mission!\n";
            return 1;
        }
        
        // 启动任务
        std::cout << "Starting mission...\n";
        if (!mission->start()) {
            std::cerr << "Failed to start mission!\n";
            return 1;
        }
        
        std::cout << "\nMission started! Enter commands (h for help):\n";
        
        // 主循环
        std::string cmd;
        while (g_running) {
            std::cout >> cmd;
            handleInteractiveCommand(*mission, cmd);
        }
        
        // 等待任务完成
        mission->waitForCompletion();
        
        // 打印最终状态
        auto final_state = mission->getState();
        printMissionState(final_state);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n[Shutdown Complete]\n";
    return 0;
}
