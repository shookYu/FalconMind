/**
 * Example 39: Communication Link Monitor
 * Full implementation with link health monitoring, automatic failover, and telemetry
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <cmath>
#include <memory>

namespace comm {

enum class LinkType { TELEMETRY, VIDEO, COMMAND };
enum class LinkStatus { HEALTHY, DEGRADED, LOST };

struct LinkMetrics {
    double rssi;          // dBm
    double packetLoss;    // percentage
    double latency;       // ms
    double throughput;    // kbps
    int consecutiveLost;
};

class CommunicationLink {
public:
    CommunicationLink(const std::string& name, LinkType type)
        : name_(name), type_(type), active_(false), priority_(1) {
        metrics_ = {-70.0, 0.0, 20.0, 100.0, 0};
    }
    
    void setPriority(int p) { priority_ = p; }
    int getPriority() const { return priority_; }
    
    bool activate() {
        active_ = true;
        std::cout << "[Link] " << name_ << " activated" << std::endl;
        return true;
    }
    
    void deactivate() {
        active_ = false;
        std::cout << "[Link] " << name_ << " deactivated" << std::endl;
    }
    
    void updateMetrics(const LinkMetrics& m) { metrics_ = m; }
    
    LinkStatus getStatus() const {
        if (metrics_.packetLoss > 50.0 || metrics_.rssi < -90.0) {
            return LinkStatus::LOST;
        } else if (metrics_.packetLoss > 10.0 || metrics_.rssi < -80.0 || metrics_.latency > 200.0) {
            return LinkStatus::DEGRADED;
        }
        return LinkStatus::HEALTHY;
    }
    
    const std::string& getName() const { return name_; }
    LinkType getType() const { return type_; }
    bool isActive() const { return active_; }
    const LinkMetrics& getMetrics() const { return metrics_; }
    
private:
    std::string name_;
    LinkType type_;
    bool active_;
    int priority_;
    LinkMetrics metrics_;
};

class LinkMonitor {
public:
    void addLink(std::shared_ptr<CommunicationLink> link) {
        links_.push_back(link);
    }
    
    void monitor() {
        std::cout << "\n[Link Monitor] Checking all links..." << std::endl;
        
        for (auto& link : links_) {
            auto status = link->getStatus();
            const auto& m = link->getMetrics();
            
            std::cout << "  " << std::setw(15) << link->getName() << " | ";
            std::cout << "RSSI: " << std::fixed << std::setprecision(1) << m.rssi << "dBm | ";
            std::cout << "Loss: " << m.packetLoss << "% | ";
            std::cout << "Latency: " << m.latency << "ms | ";
            
            switch (status) {
                case LinkStatus::HEALTHY:
                    std::cout << "[HEALTHY ✓]";
                    break;
                case LinkStatus::DEGRADED:
                    std::cout << "[DEGRADED ⚠]";
                    break;
                case LinkStatus::LOST:
                    std::cout << "[LOST ✗]";
                    break;
            }
            std::cout << std::endl;
        }
    }
    
    std::shared_ptr<CommunicationLink> getBestLink(LinkType type) const {
        std::shared_ptr<CommunicationLink> best;
        int bestPriority = -1;
        
        for (const auto& link : links_) {
            if (link->getType() == type && link->getStatus() != LinkStatus::LOST) {
                if (link->getPriority() > bestPriority) {
                    best = link;
                    bestPriority = link->getPriority();
                }
            }
        }
        return best;
    }
    
    void performFailover() {
        std::cout << "\n[Failover] Checking for failover requirements..." << std::endl;
        
        for (auto& link : links_) {
            if (link->isActive() && link->getStatus() == LinkStatus::LOST) {
                std::cout << "  Link " << link->getName() << " failed, initiating failover..." << std::endl;
                link->deactivate();
                
                auto backup = getBestLink(link->getType());
                if (backup && backup != link) {
                    backup->activate();
                    std::cout << "  Switched to backup: " << backup->getName() << std::endl;
                } else {
                    std::cout << "  [WARNING] No backup link available!" << std::endl;
                }
            }
        }
    }
    
private:
    std::vector<std::shared_ptr<CommunicationLink>> links_;
};

LinkMetrics simulateMetrics(int scenario) {
    static std::mt19937 rng(42);
    std::normal_distribution<double> rssiNoise(-70.0, 5.0);
    
    LinkMetrics m;
    m.rssi = rssiNoise(rng);
    m.latency = 20.0 + (rng() % 50);
    m.throughput = 500.0 + (rng() % 200);
    
    // Simulate different scenarios
    if (scenario == 0) {  // Normal
        m.packetLoss = rng() % 5;
    } else if (scenario == 1) {  // Degraded
        m.packetLoss = 15.0 + (rng() % 10);
        m.rssi -= 15.0;
    } else {  // Lost
        m.packetLoss = 80.0;
        m.rssi = -95.0;
        m.latency = 1000.0;
    }
    
    return m;
}

} // namespace comm

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 39: Communication Link Monitor" << std::endl;
    std::cout << "  Full Implementation: Link health + Failover" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace comm;
    
    LinkMonitor monitor;
    
    // Create links
    auto primaryTelemetry = std::make_shared<CommunicationLink>("Primary 4G", LinkType::TELEMETRY);
    primaryTelemetry->setPriority(2);
    primaryTelemetry->activate();
    monitor.addLink(primaryTelemetry);
    
    auto backupTelemetry = std::make_shared<CommunicationLink>("Backup Radio", LinkType::TELEMETRY);
    backupTelemetry->setPriority(1);
    monitor.addLink(backupTelemetry);
    
    auto videoLink = std::make_shared<CommunicationLink>("Video 5.8GHz", LinkType::VIDEO);
    videoLink->activate();
    monitor.addLink(videoLink);
    
    std::cout << "[Initialization] Links configured:" << std::endl;
    std::cout << "  - Primary 4G (Telemetry)" << std::endl;
    std::cout << "  - Backup Radio (Telemetry)" << std::endl;
    std::cout << "  - Video 5.8GHz" << std::endl;
    std::cout << std::endl;
    
    // Simulate monitoring cycles
    for (int cycle = 0; cycle < 5; cycle++) {
        // Update metrics with varying scenarios
        primaryTelemetry->updateMetrics(simulateMetrics(cycle % 3));
        backupTelemetry->updateMetrics(simulateMetrics(0));
        videoLink->updateMetrics(simulateMetrics(cycle == 3 ? 2 : 0));
        
        monitor.monitor();
        monitor.performFailover();
        
        std::cout << std::endl;
    }
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Communication link monitoring demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
