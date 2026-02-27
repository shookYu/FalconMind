/**
 * FalconMindSDK - Event Reporter Node Implementation
 * 
 * 功能：
 * - 接收搜索事件和进度
 * - MQTT遥测上报
 * - WebSocket实时推送
 * - 本地日志持久化
 * - 批量上报优化
 */

#include "falconmind/sdk/mission/EventReporterNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Bus.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <queue>
#include <cmath>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#endif

namespace falconmind::sdk::mission {

using namespace falconmind::sdk::core;

// ============================================================================
// MQTT客户端（简化实现）
// ============================================================================

class MqttClient {
public:
    MqttClient() = default;
    ~MqttClient() { disconnect(); }
    
    bool connect(const std::string& host, int port, const std::string& clientId) {
#ifdef __linux__
        sockFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockFd_ < 0) return false;
        
        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            close(sockFd_);
            sockFd_ = -1;
            return false;
        }
        
        struct sockaddr_in servAddr{};
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);
        memcpy(&servAddr.sin_addr.s_addr, server->h_addr, server->h_length);
        
        if (::connect(sockFd_, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
            close(sockFd_);
            sockFd_ = -1;
            return false;
        }
        
        // 发送MQTT CONNECT包
        if (!sendConnect(clientId)) {
            close(sockFd_);
            sockFd_ = -1;
            return false;
        }
        
        connected_ = true;
        return true;
#else
        return false;
#endif
    }
    
    void disconnect() {
#ifdef __linux__
        if (sockFd_ >= 0) {
            if (connected_) {
                sendDisconnect();
            }
            close(sockFd_);
            sockFd_ = -1;
        }
#endif
        connected_ = false;
    }
    
    bool publish(const std::string& topic, const std::string& message, bool retain = false) {
        if (!connected_) return false;
        
#ifdef __linux__
        // 构建MQTT PUBLISH包
        std::vector<uint8_t> packet;
        
        // Fixed header
        uint8_t header = 0x30;  // PUBLISH
        if (retain) header |= 0x01;
        packet.push_back(header);
        
        // Variable header + payload length
        size_t remainingLen = 2 + topic.length() + message.length();
        do {
            uint8_t byte = remainingLen % 128;
            remainingLen /= 128;
            if (remainingLen > 0) byte |= 0x80;
            packet.push_back(byte);
        } while (remainingLen > 0);
        
        // Topic length and topic
        packet.push_back((topic.length() >> 8) & 0xFF);
        packet.push_back(topic.length() & 0xFF);
        packet.insert(packet.end(), topic.begin(), topic.end());
        
        // Payload
        packet.insert(packet.end(), message.begin(), message.end());
        
        // Send
        ssize_t sent = send(sockFd_, packet.data(), packet.size(), 0);
        return sent == static_cast<ssize_t>(packet.size());
#else
        return false;
#endif
    }
    
    bool isConnected() const { return connected_; }

private:
    bool sendConnect(const std::string& clientId) {
#ifdef __linux__
        std::vector<uint8_t> packet;
        
        // Fixed header
        packet.push_back(0x10);  // CONNECT
        
        // Variable header + payload
        std::vector<uint8_t> payload;
        
        // Protocol name
        payload.push_back(0x00);
        payload.push_back(0x04);
        payload.push_back('M');
        payload.push_back('Q');
        payload.push_back('T');
        payload.push_back('T');
        
        // Protocol level
        payload.push_back(0x04);  // MQTT v3.1.1
        
        // Connect flags
        payload.push_back(0x02);  // Clean session
        
        // Keep alive
        payload.push_back(0x00);
        payload.push_back(0x3C);  // 60 seconds
        
        // Client ID
        payload.push_back((clientId.length() >> 8) & 0xFF);
        payload.push_back(clientId.length() & 0xFF);
        payload.insert(payload.end(), clientId.begin(), clientId.end());
        
        // Remaining length
        size_t remainingLen = payload.size();
        std::vector<uint8_t> lengthBytes;
        do {
            uint8_t byte = remainingLen % 128;
            remainingLen /= 128;
            if (remainingLen > 0) byte |= 0x80;
            lengthBytes.push_back(byte);
        } while (remainingLen > 0);
        
        packet.insert(packet.end(), lengthBytes.begin(), lengthBytes.end());
        packet.insert(packet.end(), payload.begin(), payload.end());
        
        ssize_t sent = send(sockFd_, packet.data(), packet.size(), 0);
        if (sent != static_cast<ssize_t>(packet.size())) return false;
        
        // Wait for CONNACK
        uint8_t response[4];
        ssize_t received = recv(sockFd_, response, 4, 0);
        if (received < 4) return false;
        
        return response[0] == 0x20 && response[3] == 0x00;  // CONNACK, success
#else
        return false;
#endif
    }
    
    void sendDisconnect() {
#ifdef __linux__
        uint8_t packet[] = {0xE0, 0x00};  // DISCONNECT
        send(sockFd_, packet, 2, 0);
#endif
    }

private:
    int sockFd_{-1};
    bool connected_{false};
};

// ============================================================================
// EventReporterNode实现
// ============================================================================

EventReporterNode::EventReporterNode() : Node("event_reporter"),
    mqttClient_(std::make_unique<MqttClient>()),
    batchMode_(false),
    maxBatchSize_(100),
    flushIntervalMs_(1000) {
    
    addPad(std::make_shared<Pad>("events", PadType::Sink));
    addPad(std::make_shared<Pad>("progress", PadType::Sink));
    
    // 默认配置
    uavId_ = "uav_001";
    missionId_ = "mission_unknown";
    mqttHost_ = "localhost";
    mqttPort_ = 1883;
    logFilePath_ = "./events.log";
    
    // 统计
    eventCount_ = 0;
    progressCount_ = 0;
}

EventReporterNode::~EventReporterNode() {
    stop();
}

bool EventReporterNode::configure(const std::unordered_map<std::string, std::string>& params) {
    Node::configure(params);
    
    if (params.find("uav_id") != params.end()) {
        uavId_ = params.at("uav_id");
    }
    if (params.find("mission_id") != params.end()) {
        missionId_ = params.at("mission_id");
    }
    if (params.find("mqtt_host") != params.end()) {
        mqttHost_ = params.at("mqtt_host");
    }
    if (params.find("mqtt_port") != params.end()) {
        mqttPort_ = std::stoi(params.at("mqtt_port"));
    }
    if (params.find("log_file") != params.end()) {
        logFilePath_ = params.at("log_file");
    }
    if (params.find("batch_mode") != params.end()) {
        batchMode_ = (params.at("batch_mode") == "true" || params.at("batch_mode") == "1");
    }
    if (params.find("batch_size") != params.end()) {
        maxBatchSize_ = std::stoi(params.at("batch_size"));
    }
    
    std::cout << "[EventReporterNode] Configured:" << std::endl;
    std::cout << "  UAV ID: " << uavId_ << std::endl;
    std::cout << "  Mission ID: " << missionId_ << std::endl;
    std::cout << "  MQTT: " << mqttHost_ << ":" << mqttPort_ << std::endl;
    std::cout << "  Log file: " << logFilePath_ << std::endl;
    std::cout << "  Batch mode: " << (batchMode_ ? "enabled" : "disabled") << std::endl;
    
    return true;
}

bool EventReporterNode::start() {
    Node::start();
    
    // 打开日志文件
    logFile_.open(logFilePath_, std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "[EventReporterNode] Failed to open log file: " << logFilePath_ << std::endl;
    }
    
    // 连接MQTT
    if (!mqttClient_->connect(mqttHost_, mqttPort_, uavId_)) {
        std::cerr << "[EventReporterNode] Failed to connect to MQTT broker" << std::endl;
        // 继续运行，使用本地日志
    } else {
        std::cout << "[EventReporterNode] Connected to MQTT broker" << std::endl;
        
        // 发送上线通知
        publishEvent("connection", "{\"status\":\"connected\",\"uav_id\":\"" + uavId_ + "\"}");
    }
    
    // 启动批量发送线程
    if (batchMode_) {
        flushThread_ = std::thread(&EventReporterNode::flushThreadFunc, this);
    }
    
    return true;
}

void EventReporterNode::stop() {
    // 刷新剩余数据
    if (batchMode_) {
        stopThread_ = true;
        if (flushThread_.joinable()) {
            flushThread_.join();
        }
        flushBatch();
    }
    
    // 发送离线通知
    if (mqttClient_->isConnected()) {
        publishEvent("connection", "{\"status\":\"disconnected\",\"uav_id\":\"" + uavId_ + "\"}");
    }
    
    mqttClient_->disconnect();
    
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    Node::stop();
    
    std::cout << "[EventReporterNode] Stopped. Events: " << eventCount_
              << ", Progress updates: " << progressCount_ << std::endl;
}

void EventReporterNode::process() {
    // 处理来自Pad的数据
}

void EventReporterNode::reportSearchEvent(const SearchEvent& event) {
    eventCount_++;
    
    // 序列化事件
    std::string json = serializeEvent(event);
    
    // 写入本地日志
    writeToLog(json);
    
    // MQTT上报
    if (batchMode_) {
        {
            std::lock_guard<std::mutex> lock(batchMutex_);
            eventBatch_.push_back(json);
        }
        if (eventBatch_.size() >= static_cast<size_t>(maxBatchSize_)) {
            flushBatch();
        }
    } else {
        publishEvent("search/event", json);
    }
    
    // 通过Bus发布
    BusMessage msg;
    msg.category = "search/event";
    msg.text = json;
    // TODO: Get Bus instance and post message
    
    // 输出到控制台
    std::cout << "[EventReporter] Event #" << eventCount_ << ": " 
              << event.description << std::endl;
}

void EventReporterNode::reportSearchProgress(const SearchProgress& progress) {
    progressCount_++;
    
    // 序列化进度
    std::string json = serializeProgress(progress);
    
    // 写入本地日志
    writeToLog(json);
    
    // MQTT上报
    publishEvent("search/progress", json);
    
    // 通过Bus发布
    BusMessage msg;
    msg.category = "search/progress";
    msg.text = json;
    // TODO: Get Bus instance and post message
    
    // 定期输出到控制台
    if (progressCount_ % 10 == 0) {
        std::cout << "[EventReporter] Progress: " << std::fixed << std::setprecision(1)
                  << (progress.coveragePercent * 100.0) << "% ("
                  << progress.waypointIndex << "/" << progress.totalWaypoints << ")"
                  << std::endl;
    }
}

void EventReporterNode::reportDetection(const std::string& targetClass, double confidence,
                                        double lat, double lon, double alt) {
    SearchEvent event;
    event.type = SearchEventType::TARGET_DETECTED;
    event.description = "Target detected: " + targetClass;
    event.position = {lat, lon, alt};
    
    auto now = std::chrono::system_clock::now();
    event.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // 构建元数据
    std::stringstream metadata;
    metadata << "{"
              << "\"class\":\"" << targetClass << "\","
              << "\"confidence\":" << confidence << ","
              << "\"uav_id\":\"" << uavId_ << "\","
              << "\"mission_id\":\"" << missionId_ << "\""
              << "}";
    event.metadata = metadata.str();
    
    reportSearchEvent(event);
}

// ============================================================================
// 序列化
// ============================================================================

std::string EventReporterNode::serializeEvent(const SearchEvent& event) {
    std::stringstream ss;
    ss << "{"
       << "\"type\":\"event\","
       << "\"event_type\":" << static_cast<int>(event.type) << ","
       << "\"description\":\"" << escapeJson(event.description) << "\","
       << "\"position\":{"
       << "\"lat\":" << std::fixed << std::setprecision(7) << event.position.lat << ","
       << "\"lon\":" << event.position.lon << ","
       << "\"alt\":" << event.position.alt
       << "},"
       << "\"timestamp_ns\":" << event.timestampNs << ","
       << "\"uav_id\":\"" << uavId_ << "\","
       << "\"mission_id\":\"" << missionId_ << "\"";
    
    if (!event.metadata.empty()) {
        ss << ",\"metadata\":" << event.metadata;
    }
    
    ss << "}";
    return ss.str();
}

std::string EventReporterNode::serializeProgress(const SearchProgress& progress) {
    std::stringstream ss;
    ss << "{"
       << "\"type\":\"progress\","
       << "\"coverage_percent\":" << std::fixed << std::setprecision(4) << progress.coveragePercent << ","
       << "\"waypoint_index\":" << progress.waypointIndex << ","
       << "\"total_waypoints\":" << progress.totalWaypoints << ","
       << "\"current_position\":{"
       << "\"lat\":" << std::fixed << std::setprecision(7) << progress.currentPosition.lat << ","
       << "\"lon\":" << progress.currentPosition.lon << ","
       << "\"alt\":" << progress.currentPosition.alt
       << "},"
       << "\"timestamp_ns\":" << std::chrono::duration_cast<std::chrono::nanoseconds>(
           std::chrono::system_clock::now().time_since_epoch()).count() << ","
       << "\"uav_id\":\"" << uavId_ << "\","
       << "\"mission_id\":\"" << missionId_ << "\""
       << "}";
    return ss.str();
}

std::string EventReporterNode::escapeJson(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

// ============================================================================
// 数据持久化和上报
// ============================================================================

void EventReporterNode::writeToLog(const std::string& json) {
    if (logFile_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        logFile_ << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " " 
                << json << std::endl;
        logFile_.flush();
    }
}

void EventReporterNode::publishEvent(const std::string& topic, const std::string& message) {
    if (mqttClient_ && mqttClient_->isConnected()) {
        std::string fullTopic = "falconmind/" + uavId_ + "/" + topic;
        if (!mqttClient_->publish(fullTopic, message)) {
            std::cerr << "[EventReporterNode] Failed to publish to " << fullTopic << std::endl;
        }
    }
}

void EventReporterNode::flushBatch() {
    std::unique_lock<std::mutex> lock(batchMutex_);
    
    if (eventBatch_.empty()) return;
    
    // Copy batch and clear under lock
    auto batchCopy = eventBatch_;
    eventBatch_.clear();
    lock.unlock();
    
    // 构建批量消息
    std::stringstream ss;
    ss << "{\"type\":\"batch\",\"events\":[";
    for (size_t i = 0; i < batchCopy.size(); ++i) {
        if (i > 0) ss << ",";
        ss << batchCopy[i];
    }
    ss << "],\"count\":" << eventBatch_.size() << "}";
    
    publishEvent("search/batch", ss.str());
}

void EventReporterNode::flushThreadFunc() {
    while (!stopThread_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(flushIntervalMs_));
        
        bool shouldFlush = false;
        {
            std::lock_guard<std::mutex> lock(batchMutex_);
            shouldFlush = !eventBatch_.empty();
        }
        if (shouldFlush) {
            flushBatch();
        }
    }
}

} // namespace falconmind::sdk::mission
