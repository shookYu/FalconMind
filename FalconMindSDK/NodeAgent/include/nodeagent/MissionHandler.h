/**
 * @file MissionHandler.h
 * @brief 任务处理器（解耦版本）
 * 
 * 使用解耦接口调用 SDK 能力，不直接依赖 SDK 类
 */

#pragma once

#include "nodeagent/DownlinkClient.h"
#include "nodeagent/sdk/SdkInterface.h"
#include <string>
#include <memory>

namespace nodeagent {

/**
 * @brief 任务处理器
 * 
 * 将下行 Mission 消息转换为 SDK 任务并执行
 */
class MissionHandler {
public:
    MissionHandler();
    ~MissionHandler();

    /**
     * @brief 设置飞控连接服务
     */
    void setFlightConnectionService(sdk::IFlightConnectionService* service);
    
    /**
     * @brief 设置任务执行服务
     */
    void setMissionExecutionService(sdk::IMissionExecutionService* service);

    /**
     * @brief 处理下行任务消息
     * @return 是否成功处理
     */
    bool handleMission(const DownlinkMessage& msg);

    /**
     * @brief 更新任务执行（每帧调用）
     */
    void update();
    
    /**
     * @brief 获取当前任务ID
     */
    const std::string& getCurrentMissionId() const { return currentMissionId_; }
    
    /**
     * @brief 检查是否有任务正在执行
     */
    bool isMissionActive() const { return missionActive_; }

private:
    /**
     * @brief 解析 JSON 任务消息
     */
    bool parseMissionJson(const std::string& jsonPayload, 
                         sdk::SearchMissionParams& params,
                         std::string& missionId);
    
    /**
     * @brief 执行回调（由 SDK 调用）
     */
    static void onMissionStatusCallback(const char* missionId, 
                                        sdk::MissionStatus status, 
                                        int progress,
                                        void* userData);

    sdk::IFlightConnectionService* flightService_{nullptr};
    sdk::IMissionExecutionService* missionService_{nullptr};
    
    std::string currentMissionId_;
    bool missionActive_{false};
};

} // namespace nodeagent
