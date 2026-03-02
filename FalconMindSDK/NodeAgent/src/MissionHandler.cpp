/**
 * @file MissionHandler.cpp
 * @brief 任务处理器实现（解耦版本）
 * 
 * 使用解耦接口调用 SDK 能力，不直接依赖 SDK 类
 */

#include "nodeagent/MissionHandler.h"
#include "nodeagent/sdk/SdkInterface.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace nodeagent {

using namespace nodeagent::sdk;

MissionHandler::MissionHandler()
    : flightService_(nullptr)
    , missionService_(nullptr)
    , currentMissionId_("")
    , missionActive_(false)
{}

MissionHandler::~MissionHandler() {
    if (missionActive_ && missionService_ && !currentMissionId_.empty()) {
        missionService_->destroyMission(currentMissionId_.c_str());
    }
}

void MissionHandler::setFlightConnectionService(IFlightConnectionService* service) {
    flightService_ = service;
}

void MissionHandler::setMissionExecutionService(IMissionExecutionService* service) {
    missionService_ = service;
    
    // 设置任务状态回调
    if (missionService_) {
        missionService_->setMissionCallback([](const char* missionId, MissionStatus status, int progress) {
            std::cout << "[MissionHandler] Mission " << missionId 
                      << " status: " << static_cast<int>(status)
                      << " progress: " << progress << "%" << std::endl;
        });
    }
}

bool MissionHandler::handleMission(const DownlinkMessage& msg) {
    if (!flightService_) {
        std::cerr << "[MissionHandler] FlightConnectionService not set" << std::endl;
        return false;
    }
    
    if (!missionService_) {
        std::cerr << "[MissionHandler] MissionExecutionService not set" << std::endl;
        return false;
    }
    
    // 解析任务参数
    SearchMissionParams params;
    std::string missionId;
    
    if (!parseMissionJson(msg.payload, params, missionId)) {
        std::cerr << "[MissionHandler] Failed to parse mission JSON" << std::endl;
        return false;
    }
    
    // 销毁之前的任务（如果有）
    if (!currentMissionId_.empty()) {
        missionService_->destroyMission(currentMissionId_.c_str());
    }
    
    // 创建新任务
    if (!missionService_->createSearchMission(missionId.c_str(), params, flightService_)) {
        std::cerr << "[MissionHandler] Failed to create mission" << std::endl;
        return false;
    }
    
    currentMissionId_ = missionId;
    
    // 启动任务
    if (!missionService_->startMission(missionId.c_str())) {
        std::cerr << "[MissionHandler] Failed to start mission" << std::endl;
        missionService_->destroyMission(missionId.c_str());
        currentMissionId_.clear();
        return false;
    }
    
    missionActive_ = true;
    std::cout << "[MissionHandler] Mission " << missionId << " started" << std::endl;
    return true;
}

void MissionHandler::update() {
    if (!missionActive_ || !missionService_ || currentMissionId_.empty()) {
        return;
    }
    
    // 检查任务状态
    MissionStatus status = missionService_->getMissionStatus(currentMissionId_.c_str());
    
    if (status == MissionStatus::Completed || 
        status == MissionStatus::Failed ||
        status == MissionStatus::Cancelled) {
        std::cout << "[MissionHandler] Mission " << currentMissionId_ 
                  << " finished with status: " << static_cast<int>(status) << std::endl;
        missionActive_ = false;
        
        // 清理任务
        missionService_->destroyMission(currentMissionId_.c_str());
        currentMissionId_.clear();
    }
}

bool MissionHandler::parseMissionJson(const std::string& jsonPayload, 
                                      SearchMissionParams& params,
                                      std::string& missionId) {
    try {
        auto json = nlohmann::json::parse(jsonPayload);
        
        // 提取任务 ID
        if (json.contains("requestId")) {
            missionId = json["requestId"].get<std::string>();
        } else {
            missionId = "mission_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        }
        
        // 提取参数
        if (json.contains("params") && json["params"].is_object()) {
            auto p = json["params"];
            
            // 搜索区域
            if (p.contains("area") && p["area"].is_object()) {
                auto area = p["area"];
                
                // 解析多边形坐标
                if (area.contains("polygon") && area["polygon"].is_array()) {
                    for (const auto& vertex : area["polygon"]) {
                        if (vertex.is_array() && vertex.size() >= 2) {
                            double lat = vertex[0].get<double>();
                            double lon = vertex[1].get<double>();
                            params.area.polygon.push_back({lat, lon});
                        }
                    }
                }
                
                params.area.minAltitude = area.value("minAltitude", 0.0);
                params.area.maxAltitude = area.value("maxAltitude", 100.0);
            }
            
            // 搜索模式
            params.pattern = p.value("pattern", "LAWN_MOWER").c_str();
            
            // 飞行参数
            params.altitude = p.value("altitude", 50.0);
            params.speed = p.value("speed", 5.0f);
            params.overlapRatio = p.value("overlapRatio", 0.2f);
            
            // 检测参数
            params.enableDetection = p.value("detectionEnabled", true);
            params.confidenceThreshold = p.value("confidenceThreshold", 0.5f);
            
            // 目标类别
            if (p.contains("targetClasses") && p["targetClasses"].is_array()) {
                for (const auto& cls : p["targetClasses"]) {
                    if (cls.is_string()) {
                        targetClasses_.push_back(cls.get<std::string>());
                        params.targetClasses.push_back(targetClasses_.back().c_str());
                    }
                }
            } else {
                // 默认目标
                targetClasses_.push_back("person");
                targetClasses_.push_back("vehicle");
                params.targetClasses.push_back(targetClasses_[0].c_str());
                params.targetClasses.push_back(targetClasses_[1].c_str());
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[MissionHandler] JSON parse error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace nodeagent
