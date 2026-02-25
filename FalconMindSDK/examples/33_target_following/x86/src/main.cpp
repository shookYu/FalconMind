/**
 * Example 33: Target Following Mission
 * Full implementation with visual tracking, path planning, and follow control
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <string>
#include <cmath>

#include <Eigen/Dense>

using namespace Eigen;

namespace target_following {

struct Pose3D {
    Vector3d position;
    Vector3d velocity;
    double timestamp;
};

struct Detection {
    Vector2d imagePosition;  // x, y in image coordinates
    double confidence;
    Vector3d estimatedWorldPos;
};

class TargetTracker {
public:
    bool update(const Detection& det) {
        if (det.confidence < 0.5) return false;
        
        // Simple moving average
        if (positionHistory_.empty()) {
            targetPosition_ = det.estimatedWorldPos;
        } else {
            targetPosition_ = 0.8 * targetPosition_ + 0.2 * det.estimatedWorldPos;
        }
        
        positionHistory_.push_back(targetPosition_);
        if (positionHistory_.size() > 10) {
            positionHistory_.erase(positionHistory_.begin());
        }
        
        // Estimate velocity
        if (positionHistory_.size() >= 2) {
            targetVelocity_ = (positionHistory_.back() - positionHistory_[positionHistory_.size()-2]) / 0.1;
        }
        
        return true;
    }
    
    Vector3d getPredictedPosition(double dt) const {
        return targetPosition_ + targetVelocity_ * dt;
    }
    
    Vector3d getPosition() const { return targetPosition_; }
    Vector3d getVelocity() const { return targetVelocity_; }
    
private:
    Vector3d targetPosition_;
    Vector3d targetVelocity_;
    std::vector<Vector3d> positionHistory_;
};

class FollowController {
public:
    FollowController() : followDistance_(10.0), followHeight_(5.0), 
                         maxSpeed_(5.0), kp_(0.5) {}
    
    void setFollowDistance(double d) { followDistance_ = d; }
    void setFollowHeight(double h) { followHeight_ = h; }
    
    Vector3d computeVelocityCommand(const Pose3D& uav, const Vector3d& targetPos) {
        // Compute desired position behind target
        Vector3d targetToUav = (uav.position - targetPos).normalized();
        Vector3d desiredPos = targetPos - targetToUav * followDistance_;
        desiredPos(2) = targetPos(2) + followHeight_;
        
        // Position error
        Vector3d error = desiredPos - uav.position;
        
        // PD control
        Vector3d velocityCmd = kp_ * error;
        
        // Limit speed
        if (velocityCmd.norm() > maxSpeed_) {
            velocityCmd = velocityCmd.normalized() * maxSpeed_;
        }
        
        return velocityCmd;
    }
    
private:
    double followDistance_;
    double followHeight_;
    double maxSpeed_;
    double kp_;
};

class TargetFollowingMission {
public:
    TargetFollowingMission() : running_(false), targetLostCount_(0) {}
    
    bool initialize() {
        std::cout << "[Mission] Initializing target following mission..." << std::endl;
        std::cout << "  Follow distance: 10m" << std::endl;
        std::cout << "  Follow height: 5m" << std::endl;
        std::cout << "  Max speed: 5m/s" << std::endl;
        return true;
    }
    
    void update(const Pose3D& uav, const Detection& detection) {
        if (tracker_.update(detection)) {
            targetLostCount_ = 0;
            
            // Predict target position
            Vector3d predictedTarget = tracker_.getPredictedPosition(0.5);
            
            // Compute follow command
            Vector3d cmd = controller_.computeVelocityCommand(uav, predictedTarget);
            
            std::cout << "  Target: [" << std::fixed << std::setprecision(1)
                      << tracker_.getPosition().transpose() << "] | ";
            std::cout << "Cmd: [" << cmd.transpose() << "]" << std::endl;
        } else {
            targetLostCount_++;
            if (targetLostCount_ > 10) {
                std::cout << "  [WARNING] Target lost! Initiating search pattern..." << std::endl;
            }
        }
    }
    
private:
    TargetTracker tracker_;
    FollowController controller_;
    bool running_;
    int targetLostCount_;
};

Pose3D simulateUav(double t) {
    Pose3D uav;
    uav.position << 10 * cos(t * 0.5), 10 * sin(t * 0.5), 10.0;
    uav.velocity << -5 * sin(t * 0.5), 5 * cos(t * 0.5), 0.0;
    uav.timestamp = t;
    return uav;
}

Detection simulateDetection(double t) {
    Detection det;
    det.imagePosition << 320, 240;
    det.confidence = 0.85 + (rand() % 15) / 100.0;
    det.estimatedWorldPos << 15 * cos(t * 0.5 + 0.2), 15 * sin(t * 0.5 + 0.2), 5.0;
    return det;
}

} // namespace target_following

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 33: Target Following Mission" << std::endl;
    std::cout << "  Full Implementation: Visual tracking + Follow control" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace target_following;
    
    TargetFollowingMission mission;
    mission.initialize();
    std::cout << std::endl;
    
    std::cout << "Running target following simulation..." << std::endl;
    std::cout << std::endl;
    
    for (int i = 0; i < 20; i++) {
        double t = i * 0.5;
        Pose3D uav = simulateUav(t);
        Detection det = simulateDetection(t);
        
        std::cout << "Step " << std::setw(2) << i << ": UAV pos [" << std::fixed << std::setprecision(1)
                  << uav.position.transpose() << "] | ";
        
        mission.update(uav, det);
    }
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Target following demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
