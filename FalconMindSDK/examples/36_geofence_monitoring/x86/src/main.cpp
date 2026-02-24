/**
 * FalconMindSDK 示例36：地理围栏监控 - 真实SDK实现
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Bus.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/sensors/GnssSourceNode.h"

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

struct GeoPoint {
    double latitude;
    double longitude;
};

class GeofenceMonitorNode : public Node {
public:
    explicit GeofenceMonitorNode(const std::string& nodeId)
        : Node(nodeId), centerLat_(39.9042), centerLon_(116.4074), 
          radius_(500.0), isInside_(true) {
        
        auto inPad = std::make_shared<Pad>("gnss_in", PadType::Sink);
        addPad(inPad);
        
        Bus::subscribe([this](const BusMessage& msg) {
            if (msg.category == "gnss/position") {
                processGNSSData(msg.text);
            }
        });
        
        std::cout << "[GeofenceMonitorNode] 初始化完成" << std::endl;
    }
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override {
        if (params.find("center_lat") != params.end()) {
            centerLat_ = std::stod(params.at("center_lat"));
        }
        if (params.find("center_lon") != params.end()) {
            centerLon_ = std::stod(params.at("center_lon"));
        }
        if (params.find("radius") != params.end()) {
            radius_ = std::stod(params.at("radius"));
        }
        return true;
    }
    
    bool start() override {
        std::cout << "[GeofenceMonitorNode] 启动围栏监控" << std::endl;
        std::cout << "  中心: (" << centerLat_ << ", " << centerLon_ << ")" << std::endl;
        std::cout << "  半径: " << radius_ << "m" << std::endl;
        return true;
    }
    
    void stop() override {
        std::cout << "[GeofenceMonitorNode] 停止围栏监控" << std::endl;
    }
    
    void process() override {
        if (currentLat_ == 0.0 && currentLon_ == 0.0) return;
        
        double distance = calculateDistance(currentLat_, currentLon_, centerLat_, centerLon_);
        bool inside = (distance <= radius_);
        
        if (!inside && isInside_) {
            std::cout << "[ALERT] 越界告警! 距离围栏中心: " << distance << "m" << std::endl;
            BusMessage alert;
            alert.category = "geofence/alert";
            alert.text = "OUT:" + std::to_string(currentLat_) + ":" + std::to_string(currentLon_);
            Bus::post(alert);
            isInside_ = false;
        } else if (inside && !isInside_) {
            std::cout << "[INFO] 返回围栏内" << std::endl;
            isInside_ = true;
        }
    }
    
private:
    double centerLat_, centerLon_, radius_;
    double currentLat_ = 0.0, currentLon_ = 0.0;
    bool isInside_;
    
    void processGNSSData(const std::string& data) {
        size_t pos = data.find(',');
        if (pos != std::string::npos) {
            currentLat_ = std::stod(data.substr(0, pos));
            currentLon_ = std::stod(data.substr(pos + 1));
        }
    }
    
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371000;
        const double toRad = 3.14159 / 180.0;
        double dLat = (lat2 - lat1) * toRad;
        double dLon = (lon2 - lon1) * toRad;
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(lat1 * toRad) * std::cos(lat2 * toRad) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        return R * 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    }
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "              FalconMindSDK 示例36: 地理围栏监控 (真实SDK机制)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    PipelineConfig config{"geofence_pipeline", "地理围栏监控", "GPS位置围栏检测"};
    auto pipeline = std::make_shared<Pipeline>(config);
    
    NodeFactory::registerNodeType("GeofenceMonitorNode", [](const std::string& id, const void*) {
        return std::make_shared<GeofenceMonitorNode>(id);
    });
    
    auto gnssNode = std::make_shared<GnssSourceNode>();
    auto geofenceNode = NodeFactory::createNode("GeofenceMonitorNode", "geofence_monitor", nullptr);
    
    auto geofenceCtrl = std::dynamic_pointer_cast<GeofenceMonitorNode>(geofenceNode);
    if (geofenceCtrl) {
        std::unordered_map<std::string, std::string> params = {
            {"center_lat", "39.9042"},
            {"center_lon", "116.4074"},
            {"radius", "500"}
        };
        geofenceCtrl->configure(params);
    }
    
    pipeline->addNode(gnssNode);
    pipeline->addNode(geofenceNode);
    
    gnssNode->start();
    geofenceNode->start();
    
    std::vector<std::pair<double, double>> testPositions = {
        {39.9042, 116.4074},
        {39.9045, 116.4078},
        {39.9100, 116.4100},
        {39.9043, 116.4075},
    };
    
    for (size_t i = 0; i < testPositions.size(); ++i) {
        std::cout << std::endl << "--- 位置更新 #" << (i + 1) << " ---" << std::endl;
        
        std::string posData = std::to_string(testPositions[i].first) + "," + 
                             std::to_string(testPositions[i].second);
        
        BusMessage gnssMsg;
        gnssMsg.category = "gnss/position";
        gnssMsg.text = posData;
        Bus::post(gnssMsg);
        
        gnssNode->process();
        geofenceNode->process();
    }
    
    gnssNode->stop();
    geofenceNode->stop();
    
    std::cout << std::endl << "================================================================================" << std::endl;
    std::cout << "                         测试完成" << std::endl;
    std::cout << "  ✓ Pipeline流程编排" << std::endl;
    std::cout << "  ✓ Node基类继承与扩展" << std::endl;
    std::cout << "  ✓ GnssSourceNode" << std::endl;
    std::cout << "  ✓ Bus消息总线" << std::endl;
    std::cout << "  ✓ Haversine距离计算" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
