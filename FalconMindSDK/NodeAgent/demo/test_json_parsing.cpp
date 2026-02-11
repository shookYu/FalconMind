// 测试 JSON 解析功能（使用 nlohmann/json）
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== JSON 解析功能测试 ===" << std::endl;

    // 测试 1: 解析命令 JSON
    std::cout << "\n[测试 1] 解析命令 JSON" << std::endl;
    try {
        std::string cmdJson = R"({"type":"ARM","uavId":"uav0","targetAlt":10.5})";
        auto json = nlohmann::json::parse(cmdJson);
        
        std::string type = json["type"].get<std::string>();
        std::string uavId = json.value("uavId", std::string("uav0"));
        double targetAlt = json.value("targetAlt", 0.0);
        
        std::cout << "  ✅ 解析成功" << std::endl;
        std::cout << "  - type: " << type << std::endl;
        std::cout << "  - uavId: " << uavId << std::endl;
        std::cout << "  - targetAlt: " << targetAlt << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  ❌ 解析失败: " << e.what() << std::endl;
        return 1;
    }

    // 测试 2: 解析任务 JSON
    std::cout << "\n[测试 2] 解析任务 JSON" << std::endl;
    try {
        std::string missionJson = R"({"id":"mission1","task":"takeoff_and_hover","params":{"takeoffAlt":15.0,"hoverDuration":10}})";
        auto json = nlohmann::json::parse(missionJson);
        
        std::string task = json["task"].get<std::string>();
        double takeoffAlt = 10.0;
        int hoverDuration = 5;
        
        if (json.contains("params") && json["params"].is_object()) {
            auto params = json["params"];
            if (params.contains("takeoffAlt")) {
                takeoffAlt = params["takeoffAlt"].get<double>();
            }
            if (params.contains("hoverDuration")) {
                hoverDuration = params["hoverDuration"].get<int>();
            }
        }
        
        std::cout << "  ✅ 解析成功" << std::endl;
        std::cout << "  - task: " << task << std::endl;
        std::cout << "  - takeoffAlt: " << takeoffAlt << std::endl;
        std::cout << "  - hoverDuration: " << hoverDuration << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  ❌ 解析失败: " << e.what() << std::endl;
        return 1;
    }

    // 测试 3: 序列化 Telemetry JSON
    std::cout << "\n[测试 3] 序列化 Telemetry JSON" << std::endl;
    try {
        nlohmann::json json;
        json["uav_id"] = "uav0";
        json["timestamp_ns"] = 1234567890;
        json["position"]["lat"] = 40.2265;
        json["position"]["lon"] = 116.2317;
        json["position"]["alt"] = 100.0;
        json["attitude"]["roll"] = 0.1;
        json["attitude"]["pitch"] = 0.2;
        json["attitude"]["yaw"] = 1.57;
        json["velocity"]["vx"] = 5.0;
        json["velocity"]["vy"] = 0.0;
        json["velocity"]["vz"] = 0.0;
        json["battery"]["percent"] = 85.0;
        json["battery"]["voltage_mv"] = 12600;
        json["gps"]["fix_type"] = 3;
        json["gps"]["num_sat"] = 12;
        json["link_quality"] = 95.0;
        json["flight_mode"] = "OFFBOARD";

        std::string serialized = json.dump();
        std::cout << "  ✅ 序列化成功" << std::endl;
        std::cout << "  - JSON 长度: " << serialized.length() << " 字节" << std::endl;
        std::cout << "  - JSON 预览: " << serialized.substr(0, 100) << "..." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  ❌ 序列化失败: " << e.what() << std::endl;
        return 1;
    }

    // 测试 4: 错误处理
    std::cout << "\n[测试 4] 错误处理（无效 JSON）" << std::endl;
    try {
        std::string invalidJson = R"({"type":"ARM"invalid})";
        auto json = nlohmann::json::parse(invalidJson);
        std::cerr << "  ❌ 应该抛出异常但没有" << std::endl;
        return 1;
    } catch (const nlohmann::json::parse_error& e) {
        std::cout << "  ✅ 正确捕获 JSON 解析错误" << std::endl;
        std::cout << "  - 错误信息: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "  ✅ 捕获到异常: " << e.what() << std::endl;
    }

    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
