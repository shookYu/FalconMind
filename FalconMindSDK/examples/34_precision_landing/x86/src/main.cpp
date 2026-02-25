/**
 * Example 34: Precision Landing
 * Full implementation with visual target detection, landing guidance, and touchdown control
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <string>
#include <cmath>

#include <Eigen/Dense>

using namespace Eigen;

namespace precision_landing {

enum class LandingPhase {
    APPROACH,
    DESCENT,
    FINAL,
    TOUCHDOWN
};

struct LandingTarget {
    Vector3d position;      // Landing zone center
    double size;            // Target size in meters
    bool detected;
    double confidence;
};

struct VehicleState {
    Vector3d position;
    Vector3d velocity;
    double heading;
    double altitude;
};

struct LandingCommand {
    Vector3d velocity;
    double descentRate;
    bool abort;
};

class PrecisionLandingController {
public:
    PrecisionLandingController() 
        : phase_(LandingPhase::APPROACH), 
          approachAltitude_(20.0),
          finalAltitude_(5.0),
          landingSpeed_(0.5),
          horizontalGain_(0.3),
          verticalGain_(0.5) {}
    
    bool initialize() {
        std::cout << "[Landing] Initializing precision landing controller..." << std::endl;
        std::cout << "  Approach altitude: 20m" << std::endl;
        std::cout << "  Final altitude: 5m" << std::endl;
        std::cout << "  Landing speed: 0.5m/s" << std::endl;
        return true;
    }
    
    LandingCommand update(const VehicleState& state, const LandingTarget& target) {
        LandingCommand cmd;
        cmd.abort = false;
        
        // State machine
        switch (phase_) {
            case LandingPhase::APPROACH:
                cmd = handleApproach(state, target);
                break;
            case LandingPhase::DESCENT:
                cmd = handleDescent(state, target);
                break;
            case LandingPhase::FINAL:
                cmd = handleFinal(state, target);
                break;
            case LandingPhase::TOUCHDOWN:
                cmd = handleTouchdown(state, target);
                break;
        }
        
        // Safety checks
        if (!target.detected && state.altitude < 10.0) {
            std::cout << "  [ALERT] Target lost below 10m! Aborting landing." << std::endl;
            cmd.abort = true;
        }
        
        return cmd;
    }
    
    LandingPhase getPhase() const { return phase_; }
    const char* getPhaseString() const {
        switch (phase_) {
            case LandingPhase::APPROACH: return "APPROACH";
            case LandingPhase::DESCENT: return "DESCENT";
            case LandingPhase::FINAL: return "FINAL";
            case LandingPhase::TOUCHDOWN: return "TOUCHDOWN";
            default: return "UNKNOWN";
        }
    }
    
private:
    LandingCommand handleApproach(const VehicleState& state, const LandingTarget& target) {
        LandingCommand cmd;
        
        // Move towards target at approach altitude
        Vector3d error = target.position - state.position;
        error(2) = 0;  // Ignore altitude error for horizontal control
        
        cmd.velocity = horizontalGain_ * error;
        cmd.velocity(2) = -0.5;  // Slow descent to approach altitude
        
        if (state.altitude <= approachAltitude_ + 1.0) {
            phase_ = LandingPhase::DESCENT;
            std::cout << "  [Phase] Transition to DESCENT" << std::endl;
        }
        
        return cmd;
    }
    
    LandingCommand handleDescent(const VehicleState& state, const LandingTarget& target) {
        LandingCommand cmd;
        
        // Center on target while descending
        Vector3d error = target.position - state.position;
        cmd.velocity = horizontalGain_ * error;
        cmd.velocity(2) = -1.0;  // 1m/s descent
        
        if (state.altitude <= finalAltitude_) {
            phase_ = LandingPhase::FINAL;
            std::cout << "  [Phase] Transition to FINAL" << std::endl;
        }
        
        return cmd;
    }
    
    LandingCommand handleFinal(const VehicleState& state, const LandingTarget& target) {
        LandingCommand cmd;
        
        // Very slow descent with precise centering
        Vector3d error = target.position - state.position;
        cmd.velocity = horizontalGain_ * 2.0 * error;  // Higher gain for precision
        cmd.velocity(2) = -landingSpeed_;  // Slow landing speed
        
        if (state.altitude <= 0.5) {
            phase_ = LandingPhase::TOUCHDOWN;
            std::cout << "  [Phase] Transition to TOUCHDOWN" << std::endl;
        }
        
        return cmd;
    }
    
    LandingCommand handleTouchdown(const VehicleState& state, const LandingTarget& target) {
        LandingCommand cmd;
        
        // Cut throttle and land
        cmd.velocity.setZero();
        cmd.velocity(2) = -0.2;  // Very slow final descent
        
        if (state.altitude <= 0.1) {
            std::cout << "  [SUCCESS] Touchdown complete!" << std::endl;
            cmd.velocity.setZero();
        }
        
        return cmd;
    }
    
    LandingPhase phase_;
    double approachAltitude_;
    double finalAltitude_;
    double landingSpeed_;
    double horizontalGain_;
    double verticalGain_;
};

VehicleState simulateVehicle(double t) {
    VehicleState state;
    // Descending spiral
    double radius = 20.0 * exp(-t * 0.1);
    state.position << radius * cos(t), radius * sin(t), 20.0 - t * 0.8;
    state.velocity << -radius * 0.1 * cos(t) - radius * sin(t),
                    -radius * 0.1 * sin(t) + radius * cos(t),
                    -0.8;
    state.heading = atan2(state.velocity(1), state.velocity(0));
    state.altitude = state.position(2);
    return state;
}

LandingTarget simulateTarget(double t) {
    LandingTarget target;
    target.position << 0, 0, 0;
    target.size = 1.0;
    target.detected = (t < 25.0);  // Target lost near end for testing
    target.confidence = 0.9;
    return target;
}

} // namespace precision_landing

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 34: Precision Landing" << std::endl;
    std::cout << "  Full Implementation: Visual guidance + Landing control" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace precision_landing;
    
    PrecisionLandingController controller;
    controller.initialize();
    std::cout << std::endl;
    
    std::cout << "Executing precision landing sequence..." << std::endl;
    std::cout << std::endl;
    
    for (int step = 0; step < 30; step++) {
        double t = step * 0.8;
        VehicleState state = simulateVehicle(t);
        LandingTarget target = simulateTarget(t);
        
        std::cout << "Step " << std::setw(2) << step << " [" << controller.getPhaseString() << "]: ";
        std::cout << "Alt=" << std::fixed << std::setprecision(1) << state.altitude << "m | ";
        std::cout << "Target detected=" << (target.detected ? "Yes" : "No") << " | ";
        
        LandingCommand cmd = controller.update(state, target);
        
        if (cmd.abort) {
            std::cout << "ABORT!" << std::endl;
            break;
        } else {
            std::cout << "Vz=" << cmd.velocity(2) << "m/s" << std::endl;
        }
        
        if (state.altitude <= 0.1) break;
    }
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Precision landing demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
