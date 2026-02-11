#include "nodeagent/MultiUavManager.h"

#include <iostream>

namespace nodeagent {

MultiUavManager::MultiUavManager() {
}

MultiUavManager::~MultiUavManager() {
    stopAll();
}

bool MultiUavManager::addUav(const UavConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (uavs_.find(config.uavId) != uavs_.end()) {
        std::cerr << "[MultiUavManager] UAV already exists: " << config.uavId << std::endl;
        return false;
    }

    UavEntry entry;
    entry.config = config;

    NodeAgent::Config agentConfig;
    agentConfig.uavId = config.uavId;
    agentConfig.centerAddress = config.centerAddress;
    agentConfig.centerPort = config.centerPort;

    entry.agent = std::make_unique<NodeAgent>(agentConfig);
    if (config.flightService) {
        entry.agent->setFlightConnectionService(config.flightService);
    }

    uavs_[config.uavId] = std::move(entry);

    std::cout << "[MultiUavManager] Added UAV: " << config.uavId << std::endl;
    return true;
}

bool MultiUavManager::removeUav(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = uavs_.find(uavId);
    if (it == uavs_.end()) {
        std::cerr << "[MultiUavManager] UAV not found: " << uavId << std::endl;
        return false;
    }

    if (it->second.running) {
        it->second.agent->stop();
    }

    uavs_.erase(it);
    std::cout << "[MultiUavManager] Removed UAV: " << uavId << std::endl;
    return true;
}

bool MultiUavManager::startAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    bool allSuccess = true;
    for (auto& [uavId, entry] : uavs_) {
        if (!entry.running) {
            if (entry.agent->start()) {
                entry.running = true;
                std::cout << "[MultiUavManager] Started UAV: " << uavId << std::endl;
            } else {
                std::cerr << "[MultiUavManager] Failed to start UAV: " << uavId << std::endl;
                allSuccess = false;
            }
        }
    }

    return allSuccess;
}

void MultiUavManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [uavId, entry] : uavs_) {
        if (entry.running) {
            entry.agent->stop();
            entry.running = false;
            std::cout << "[MultiUavManager] Stopped UAV: " << uavId << std::endl;
        }
    }
}

bool MultiUavManager::startUav(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = uavs_.find(uavId);
    if (it == uavs_.end()) {
        std::cerr << "[MultiUavManager] UAV not found: " << uavId << std::endl;
        return false;
    }

    if (it->second.running) {
        return true;  // 已经在运行
    }

    if (it->second.agent->start()) {
        it->second.running = true;
        std::cout << "[MultiUavManager] Started UAV: " << uavId << std::endl;
        return true;
    }

    return false;
}

void MultiUavManager::stopUav(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = uavs_.find(uavId);
    if (it == uavs_.end()) {
        std::cerr << "[MultiUavManager] UAV not found: " << uavId << std::endl;
        return;
    }

    if (it->second.running) {
        it->second.agent->stop();
        it->second.running = false;
        std::cout << "[MultiUavManager] Stopped UAV: " << uavId << std::endl;
    }
}

std::vector<std::string> MultiUavManager::getUavList() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> list;
    for (const auto& [uavId, entry] : uavs_) {
        list.push_back(uavId);
    }

    return list;
}

bool MultiUavManager::isUavRunning(const std::string& uavId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = uavs_.find(uavId);
    if (it == uavs_.end()) {
        return false;
    }

    return it->second.running;
}

} // namespace nodeagent
