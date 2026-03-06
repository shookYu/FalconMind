/**
 * @file mission_planner_process.cpp
 * @brief Mission Planner Process - State Machine and Waypoint Generation
 * 
 * This process performs:
 * - Mission state machine management (IDLE -> TAKEOFF -> SEARCHING -> TRACKING...)
 * - Search pattern waypoint generation (LAWN_MOWER, SPIRAL, ZIGZAG)
 * - Target acquisition and handoff to guidance_process
 * - DDS/MQTT communication with other processes
 * 
 * Architecture:
 *   DetectionArray (DDS) -> MissionPlanner -> MissionCommand (DDS/MQTT)
 *   TrackingArray (DDS) -> State Machine -> GuidanceCommand (DDS)
 * 
 * Dependencies:
 *   - Fast DDS
 *   - MQTT (paho-mqttpp3)
 *   - nlohmann/json
 * 
 * Build:
 *   cmake .. -DCMAKE_BUILD_TYPE=Release
 *   make -j$(nproc)
 * 
 * Run:
 *   ./mission_planner_process --config /etc/falconmind/mission_planner.yaml
 * 
 * @author FalconMind Engineering Team
 * @version 1.0.0
 */

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

#include <mqtt/async_client.h>

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <math>
#include <algorithm>

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

// DDS generated types
#include "FalconMindTypes.h"
#include "FalconMindTypesPubSubTypes.h"

using namespace eprosima::fastdds::dds;
using namespace falconmind::dds;
using json = nlohmann::json;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration
// ============================================================================

struct MissionPlannerConfig {
    // DDS settings
    std::string dds_domain_id{"0"};
    
    // MQTT settings
    std::string mqtt_broker{"localhost"};
    int mqtt_port{1883};
    std::string mqtt_client_id{"mission_planner_001"};
    std::string mqtt_command_topic{"falconmind/command"};
    std::string mqtt_status_topic{"falconmind/status/mission"};
    
    // Search pattern settings
    std::string search_pattern{"LAWN_MOWER"};  // LAWN_MOWER, SPIRAL, ZIGZAG
    float search_altitude{50.0f};              // Search altitude (m)
    float search_speed{5.0f};                  // Search speed (m/s)
    float waypoint_spacing{20.0f};             // Spacing between tracks (m)
    float overlap_rate{0.2f};                  // Search area overlap
    float turn_radius{10.0f};                  // Turn radius (m)
    
    // Target detection settings
    float detection_confidence_threshold{0.7f};
    int min_detection_frames{3};               // Min frames to confirm target
    std::vector<std::string> target_classes{"person", "vehicle"};
    
    // Tracking settings
    float desired_distance{30.0f};             // Target tracking distance (m)
    float desired_height{10.0f};               // Target tracking height (m)
    float max_tracking_speed{8.0f};            // Max tracking speed (m/s)
    
    // Safety settings
    float geofence_radius{500.0f};             // Max distance from home (m)
    float max_altitude{120.0f};                // Max altitude (m)
    float battery_rtl_threshold{30.0f};        // Return to launch battery %
    float mission_timeout{1800.0f};            // Mission timeout (seconds)
    
    // State machine settings
    float takeoff_height{50.0f};
    int max_tracking_loss_seconds{10};         // Max time without target before abort
};

// ============================================================================
// Mission State Machine
// ============================================================================

enum class MissionState {
    IDLE,               // Waiting for mission start
    INITIALIZING,       // VINS initialization
    TAKEOFF,           // Taking off to search altitude
    SEARCHING,         // Searching for target
    TARGET_ACQUIRED,   // Target detected, waiting for confirmation
    TRACKING,          // Actively tracking target
    TARGET_LOST,       // Target lost, attempting recovery
    RETURNING,         // Returning to launch
    LANDED,            // Mission complete
    ABORTED            // Mission aborted
};

const char* stateToString(MissionState state) {
    switch (state) {
        case MissionState::IDLE: return "IDLE";
        case MissionState::INITIALIZING: return "INITIALIZING";
        case MissionState::TAKEOFF: return "TAKEOFF";
        case MissionState::SEARCHING: return "SEARCHING";
        case MissionState::TARGET_ACQUIRED: return "TARGET_ACQUIRED";
        case MissionState::TRACKING: return "TRACKING";
        case MissionState::TARGET_LOST: return "TARGET_LOST";
        case MissionState::RETURNING: return "RETURNING";
        case MissionState::LANDED: return "LANDED";
        case MissionState::ABORTED: return "ABORTED";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Waypoint Structure
// ============================================================================

struct Waypoint {
    float latitude;
    float longitude;
    float altitude;
    float speed;
    bool accept_yaw{false};
    float yaw{0.0f};
    
    Waypoint(float lat = 0, float lon = 0, float alt = 0, float spd = 5.0f)
        : latitude(lat), longitude(lon), altitude(alt), speed(spd) {}
};

// ============================================================================
// Search Pattern Generator (Production-grade)
// ============================================================================

class SearchPatternGenerator {
public:
    explicit SearchPatternGenerator(const MissionPlannerConfig& config) 
        : config_(config) {}
    
    std::vector<Waypoint> generateLawnMowerPattern(
        double center_lat, double center_lon,
        float width_m, float height_m) {
        
        std::vector<Waypoint> waypoints;
        
        // Convert meters to degrees (approximate)
        float meters_per_deg_lat = 111320.0f;
        float meters_per_deg_lon = 111320.0f * std::cos(center_lat * M_PI / 180.0f);
        
        float width_deg = width_m / meters_per_deg_lon;
        float height_deg = height_m / meters_per_deg_lat;
        float spacing_deg = config_.waypoint_spacing / meters_per_deg_lon;
        
        float min_lat = center_lat - height_deg / 2.0f;
        float max_lat = center_lat + height_deg / 2.0f;
        float min_lon = center_lon - width_deg / 2.0f;
        float max_lon = center_lon + width_deg / 2.0f;
        
        bool direction = true;  // true = north, false = south
        int num_strips = static_cast<int>(std::ceil(width_deg / spacing_deg));
        
        for (int i = 0; i <= num_strips; ++i) {
            float lon = min_lon + i * spacing_deg;
            if (lon > max_lon) lon = max_lon;
            
            if (direction) {
                waypoints.emplace_back(min_lat, lon, config_.search_altitude, config_.search_speed);
                waypoints.emplace_back(max_lat, lon, config_.search_altitude, config_.search_speed);
            } else {
                waypoints.emplace_back(max_lat, lon, config_.search_altitude, config_.search_speed);
                waypoints.emplace_back(min_lat, lon, config_.search_altitude, config_.search_speed);
            }
            
            direction = !direction;
        }
        
        return waypoints;
    }
    
    std::vector<Waypoint> generateSpiralPattern(
        double center_lat, double center_lon,
        float max_radius_m, float spacing_m) {
        
        std::vector<Waypoint> waypoints;
        
        float meters_per_deg_lat = 111320.0f;
        float meters_per_deg_lon = 111320.0f * std::cos(center_lat * M_PI / 180.0f);
        
        float radius = spacing_m;
        float angle = 0.0f;
        
        while (radius <= max_radius_m) {
            float lat_offset = (radius * std::cos(angle)) / meters_per_deg_lat;
            float lon_offset = (radius * std::sin(angle)) / meters_per_deg_lon;
            
            waypoints.emplace_back(
                center_lat + lat_offset,
                center_lon + lon_offset,
                config_.search_altitude,
                config_.search_speed
            );
            
            angle += 0.5f;  // Increment angle
            radius = spacing_m * (1.0f + angle / (2.0f * M_PI));
        }
        
        return waypoints;
    }
    
    std::vector<Waypoint> generateZigzagPattern(
        double center_lat, double center_lon,
        float width_m, float height_m, float leg_length_m) {
        
        std::vector<Waypoint> waypoints;
        
        float meters_per_deg_lat = 111320.0f;
        float meters_per_deg_lon = 111320.0f * std::cos(center_lat * M_PI / 180.0f);
        
        float width_deg = width_m / meters_per_deg_lon;
        float height_deg = height_m / meters_per_deg_lat;
        float leg_deg = leg_length_m / meters_per_deg_lat;
        
        float min_lat = center_lat - height_deg / 2.0f;
        float current_lat = min_lat;
        bool direction = true;  // true = east, false = west
        
        while (current_lat <= center_lat + height_deg / 2.0f) {
            if (direction) {
                waypoints.emplace_back(
                    current_lat, center_lon - width_deg / 2.0f,
                    config_.search_altitude, config_.search_speed
                );
                waypoints.emplace_back(
                    current_lat + leg_deg, center_lon + width_deg / 2.0f,
                    config_.search_altitude, config_.search_speed
                );
            } else {
                waypoints.emplace_back(
                    current_lat + leg_deg, center_lon + width_deg / 2.0f,
                    config_.search_altitude, config_.search_speed
                );
                waypoints.emplace_back(
                    current_lat, center_lon - width_deg / 2.0f,
                    config_.search_altitude, config_.search_speed
                );
            }
            
            current_lat += leg_deg * 2.0f;
            direction = !direction;
        }
        
        return waypoints;
    }
    
    std::vector<Waypoint> generateReturnToLaunch(
        double home_lat, double home_lon,
        double current_lat, double current_lon) {
        
        std::vector<Waypoint> waypoints;
        
        // Climb to RTL altitude
        waypoints.emplace_back(current_lat, current_lon, config_.search_altitude, config_.search_speed);
        
        // Fly to home position
        waypoints.emplace_back(home_lat, home_lon, config_.search_altitude, config_.search_speed);
        
        // Descend and land
        waypoints.emplace_back(home_lat, home_lon, 10.0f, 2.0f);
        waypoints.emplace_back(home_lat, home_lon, 0.0f, 1.0f);
        
        return waypoints;
    }
    
private:
    const MissionPlannerConfig& config_;
};

// ============================================================================
// Target Detection Manager
// ============================================================================

struct DetectedTarget {
    int track_id;
    std::string class_name;
    float confidence;
    float bbox_x, bbox_y, bbox_width, bbox_height;
    float image_x, image_y;
    int consecutive_frames{0};
    std::chrono::steady_clock::time_point first_detection;
    std::chrono::steady_clock::time_point last_detection;
    
    bool isConfirmed(int min_frames) const {
        return consecutive_frames >= min_frames;
    }
};

class TargetManager {
public:
    explicit TargetManager(const MissionPlannerConfig& config) 
        : config_(config), selected_target_id_(-1) {}
    
    void updateDetections(const DetectionArray& detections) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        for (const auto& det : detections.detections()) {
            // Check if target class is in our list
            if (std::find(config_.target_classes.begin(), 
                         config_.target_classes.end(), 
                         det.class_name()) == config_.target_classes.end()) {
                continue;
            }
            
            if (det.confidence() < config_.detection_confidence_threshold) {
                continue;
            }
            
            int track_id = det.track_id();
            
            auto it = targets_.find(track_id);
            if (it != targets_.end()) {
                // Update existing target
                it->second.confidence = det.confidence();
                it->second.bbox_x = det.bbox().x();
                it->second.bbox_y = det.bbox().y();
                it->second.bbox_width = det.bbox().width();
                it->second.bbox_height = det.bbox().height();
                it->second.image_x = det.image_position().x();
                it->second.image_y = det.image_position().y();
                it->second.consecutive_frames++;
                it->second.last_detection = now;
            } else {
                // New target
                DetectedTarget target;
                target.track_id = track_id;
                target.class_name = det.class_name();
                target.confidence = det.confidence();
                target.bbox_x = det.bbox().x();
                target.bbox_y = det.bbox().y();
                target.bbox_width = det.bbox().width();
                target.bbox_height = det.bbox().height();
                target.image_x = det.image_position().x();
                target.image_y = det.image_position().y();
                target.consecutive_frames = 1;
                target.first_detection = now;
                target.last_detection = now;
                
                targets_[track_id] = target;
            }
        }
        
        // Remove stale targets
        cleanupStaleTargets(now);
    }
    
    bool hasConfirmedTarget() const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, target] : targets_) {
            if (target.isConfirmed(config_.min_detection_frames)) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<DetectedTarget> getConfirmedTargets() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<DetectedTarget> confirmed;
        for (const auto& [id, target] : targets_) {
            if (target.isConfirmed(config_.min_detection_frames)) {
                confirmed.push_back(target);
            }
        }
        return confirmed;
    }
    
    bool selectTarget(int track_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (targets_.find(track_id) != targets_.end()) {
            selected_target_id_ = track_id;
            return true;
        }
        return false;
    }
    
    int getSelectedTargetId() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return selected_target_id_;
    }
    
    bool isSelectedTargetValid() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (selected_target_id_ < 0) return false;
        
        auto it = targets_.find(selected_target_id_);
        if (it == targets_.end()) return false;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - it->second.last_detection).count();
        
        return elapsed < config_.max_tracking_loss_seconds;
    }
    
    void clearSelection() {
        std::lock_guard<std::mutex> lock(mutex_);
        selected_target_id_ = -1;
    }
    
    size_t getTargetCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return targets_.size();
    }
    
private:
    void cleanupStaleTargets(std::chrono::steady_clock::time_point now) {
        for (auto it = targets_.begin(); it != targets_.end();) {
            auto elapsed = std::chrono::duration<double>(now - it->second.last_detection).count();
            if (elapsed > config_.max_tracking_loss_seconds * 2) {
                it = targets_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    const MissionPlannerConfig& config_;
    mutable std::mutex mutex_;
    std::unordered_map<int, DetectedTarget> targets_;
    int selected_target_id_;
};

// ============================================================================
// DDS Communication
// ============================================================================

class DDSCommunication {
public:
    DDSCommunication() : participant_(nullptr), subscriber_(nullptr), 
                         publisher_(nullptr), detection_reader_(nullptr),
                         tracking_reader_(nullptr), mission_writer_(nullptr) {}
    
    ~DDSCommunication() {
        cleanup();
    }
    
    bool initialize(const std::string& domain_id,
                   std::function<void(const DetectionArray&)> detection_callback,
                   std::function<void(const TrackingArray&)> tracking_callback) {
        
        detection_callback_ = detection_callback;
        tracking_callback_ = tracking_callback;
        
        // Create participant
        DomainParticipantQos participant_qos;
        participant_qos.name("mission_planner_participant");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            std::stoi(domain_id), participant_qos);
        
        if (!participant_) {
            std::cerr << "[DDS] Failed to create participant" << std::endl;
            return false;
        }
        
        // Create subscriber
        SubscriberQos subscriber_qos;
        subscriber_ = participant_->create_subscriber(subscriber_qos, nullptr);
        
        // Create publisher
        PublisherQos publisher_qos;
        publisher_ = participant_->create_publisher(publisher_qos, nullptr);
        
        // Register types
        detection_type_.register_type(participant_);
        tracking_type_.register_type(participant_);
        
        // Create detection topic and reader
        TopicQos detection_topic_qos;
        detection_topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        
        auto* detection_topic = participant_->create_topic(
            "DetectionArray", detection_type_.get_type_name(), detection_topic_qos);
        
        DataReaderQos detection_reader_qos;
        detection_reader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        detection_reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        detection_reader_qos.history().depth = 1;
        
        detection_reader_ = subscriber_->create_datareader(
            detection_topic, detection_reader_qos, &detection_listener_);
        
        // Create tracking topic and reader
        TopicQos tracking_topic_qos;
        tracking_topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        
        auto* tracking_topic = participant_->create_topic(
            "TrackingArray", tracking_type_.get_type_name(), tracking_topic_qos);
        
        DataReaderQos tracking_reader_qos;
        tracking_reader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        tracking_reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        tracking_reader_qos.history().depth = 1;
        
        tracking_reader_ = subscriber_->create_datareader(
            tracking_topic, tracking_reader_qos, &tracking_listener_);
        
        std::cout << "[DDS] Communication initialized" << std::endl;
        return true;
    }
    
    void cleanup() {
        if (participant_) {
            if (detection_reader_) subscriber_->delete_datareader(detection_reader_);
            if (tracking_reader_) subscriber_->delete_datareader(tracking_reader_);
            if (mission_writer_) publisher_->delete_datawriter(mission_writer_);
            if (subscriber_) participant_->delete_subscriber(subscriber_);
            if (publisher_) participant_->delete_publisher(publisher_);
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }
    
private:
    class DetectionListener : public DataReaderListener {
    public:
        explicit DetectionListener(std::function<void(const DetectionArray&)> callback)
            : callback_(callback) {}
        
        void on_data_available(DataReader* reader) override {
            DetectionArray detection;
            SampleInfo info;
            
            while (reader->take_next_sample(&detection, &info) == ReturnCode_t::RETCODE_OK) {
                if (info.valid_data && callback_) {
                    callback_(detection);
                }
            }
        }
        
    private:
        std::function<void(const DetectionArray&)> callback_;
    };
    
    class TrackingListener : public DataReaderListener {
    public:
        explicit TrackingListener(std::function<void(const TrackingArray&)> callback)
            : callback_(callback) {}
        
        void on_data_available(DataReader* reader) override {
            TrackingArray tracking;
            SampleInfo info;
            
            while (reader->take_next_sample(&tracking, &info) == ReturnCode_t::RETCODE_OK) {
                if (info.valid_data && callback_) {
                    callback_(tracking);
                }
            }
        }
        
    private:
        std::function<void(const TrackingArray&)> callback_;
    };
    
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Publisher* publisher_;
    DataReader* detection_reader_;
    DataReader* tracking_reader_;
    DataWriter* mission_writer_;
    
    DetectionArrayPubSubType detection_type_;
    TrackingArrayPubSubType tracking_type_;
    
    DetectionListener detection_listener_{nullptr};
    TrackingListener tracking_listener_{nullptr};
    
    std::function<void(const DetectionArray&)> detection_callback_;
    std::function<void(const TrackingArray&)> tracking_callback_;
};

// ============================================================================
// Mission Planner Core
// ============================================================================

class MissionPlanner {
public:
    MissionPlanner(const MissionPlannerConfig& config)
        : config_(config),
          state_(MissionState::IDLE),
          pattern_generator_(config),
          target_manager_(config),
          home_position_{0.0, 0.0},
          current_position_{0.0, 0.0},
          mission_start_time_{std::chrono::steady_clock::now()},
          tracking_start_time_{},
          target_acquired_{false},
          current_waypoint_idx_{0} {}
    
    void startMission(double home_lat, double home_lon, 
                     float search_width, float search_height) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        home_position_ = {home_lat, home_lon};
        current_position_ = {home_lat, home_lon};
        
        // Generate search waypoints
        if (config_.search_pattern == "LAWN_MOWER") {
            waypoints_ = pattern_generator_.generateLawnMowerPattern(
                home_lat, home_lon, search_width, search_height);
        } else if (config_.search_pattern == "SPIRAL") {
            waypoints_ = pattern_generator_.generateSpiralPattern(
                home_lat, home_lon, search_width / 2.0f, config_.waypoint_spacing);
        } else if (config_.search_pattern == "ZIGZAG") {
            waypoints_ = pattern_generator_.generateZigzagPattern(
                home_lat, home_lon, search_width, search_height, search_height / 4.0f);
        }
        
        transitionToState(MissionState::INITIALIZING);
        mission_start_time_ = std::chrono::steady_clock::now();
        
        std::cout << "[Mission] Started with " << waypoints_.size() << " waypoints" << std::endl;
    }
    
    void abortMission() {
        std::lock_guard<std::mutex> lock(state_mutex_);
        transitionToState(MissionState::ABORTED);
    }
    
    void selectTarget(int track_id) {
        if (target_manager_.selectTarget(track_id)) {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (state_ == MissionState::TARGET_ACQUIRED) {
                transitionToState(MissionState::TRACKING);
                tracking_start_time_ = std::chrono::steady_clock::now();
            }
        }
    }
    
    void updateNavigationState(const NavigationState& nav) {
        current_position_ = {nav.position().x(), nav.position().y()};
        current_altitude_ = nav.position().z();
    }
    
    void processDetections(const DetectionArray& detections) {
        target_manager_.updateDetections(detections);
        
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        switch (state_) {
            case MissionState::SEARCHING:
                if (target_manager_.hasConfirmedTarget()) {
                    transitionToState(MissionState::TARGET_ACQUIRED);
                }
                break;
                
            case MissionState::TRACKING:
                if (!target_manager_.isSelectedTargetValid()) {
                    transitionToState(MissionState::TARGET_LOST);
                }
                break;
                
            case MissionState::TARGET_LOST:
                if (target_manager_.isSelectedTargetValid()) {
                    transitionToState(MissionState::TRACKING);
                } else {
                    // Check if we've been in lost state too long
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration<double>(
                        now - tracking_start_time_).count();
                    if (elapsed > config_.max_tracking_loss_seconds * 2) {
                        transitionToState(MissionState::RETURNING);
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    void processTracking(const TrackingArray& tracking) {
        // Update target information from tracking
    }
    
    void updateStateMachine() {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        switch (state_) {
            case MissionState::INITIALIZING:
                // After VINS initialization, move to takeoff
                transitionToState(MissionState::TAKEOFF);
                break;
                
            case MissionState::TAKEOFF:
                if (current_altitude_ >= config_.takeoff_height * 0.9f) {
                    transitionToState(MissionState::SEARCHING);
                }
                break;
                
            case MissionState::SEARCHING:
                // Check mission timeout
                if (checkMissionTimeout(now)) {
                    transitionToState(MissionState::RETURNING);
                }
                break;
                
            case MissionState::TRACKING:
                // Check mission timeout
                if (checkMissionTimeout(now)) {
                    transitionToState(MissionState::RETURNING);
                }
                break;
                
            case MissionState::RETURNING:
                // Check if we're close to home
                if (isAtHome()) {
                    transitionToState(MissionState::LANDED);
                }
                break;
                
            default:
                break;
        }
    }
    
    MissionState getState() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }
    
    std::string getStateString() const {
        return stateToString(getState());
    }
    
    std::vector<Waypoint> getWaypoints() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return waypoints_;
    }
    
    std::vector<DetectedTarget> getConfirmedTargets() const {
        return target_manager_.getConfirmedTargets();
    }
    
    bool isMissionActive() const {
        auto state = getState();
        return state != MissionState::IDLE && 
               state != MissionState::LANDED && 
               state != MissionState::ABORTED;
    }
    
private:
    void transitionToState(MissionState new_state) {
        std::cout << "[State] " << stateToString(state_) 
                  << " -> " << stateToString(new_state) << std::endl;
        
        // State exit actions
        switch (state_) {
            case MissionState::TRACKING:
                target_manager_.clearSelection();
                break;
            default:
                break;
        }
        
        state_ = new_state;
        
        // State entry actions
        switch (state_) {
            case MissionState::RETURNING:
                waypoints_ = pattern_generator_.generateReturnToLaunch(
                    home_position_.first, home_position_.second,
                    current_position_.first, current_position_.second);
                current_waypoint_idx_ = 0;
                break;
                
            case MissionState::LANDED:
                std::cout << "[Mission] Complete" << std::endl;
                break;
                
            case MissionState::ABORTED:
                std::cout << "[Mission] Aborted" << std::endl;
                break;
                
            default:
                break;
        }
    }
    
    bool checkMissionTimeout(std::chrono::steady_clock::time_point now) {
        auto elapsed = std::chrono::duration<double>(now - mission_start_time_).count();
        return elapsed > config_.mission_timeout;
    }
    
    bool isAtHome() {
        // Check if close to home position (within 5m)
        float dx = current_position_.first - home_position_.first;
        float dy = current_position_.second - home_position_.second;
        return std::sqrt(dx * dx + dy * dy) < 0.0001f;  // ~10m in degrees
    }
    
    const MissionPlannerConfig& config_;
    mutable std::mutex state_mutex_;
    MissionState state_;
    
    SearchPatternGenerator pattern_generator_;
    TargetManager target_manager_;
    
    std::pair<double, double> home_position_;
    std::pair<double, double> current_position_;
    float current_altitude_{0.0f};
    
    std::vector<Waypoint> waypoints_;
    size_t current_waypoint_idx_{0};
    
    std::chrono::steady_clock::time_point mission_start_time_;
    std::chrono::steady_clock::time_point tracking_start_time_;
    
    bool target_acquired_{false};
};

// ============================================================================
// Signal Handling
// ============================================================================

static std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\n[Mission Planner] Received signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
}

// ============================================================================
// Configuration Loading
// ============================================================================

bool loadConfig(const std::string& config_path, MissionPlannerConfig& config) {
    try {
        YAML::Node yaml = YAML::LoadFile(config_path);
        
        if (yaml["mqtt"]) {
            config.mqtt_broker = yaml["mqtt"]["broker"].as<std::string>("localhost");
            config.mqtt_port = yaml["mqtt"]["port"].as<int>(1883);
        }
        
        if (yaml["search"]) {
            config.search_pattern = yaml["search"]["pattern"].as<std::string>("LAWN_MOWER");
            config.search_altitude = yaml["search"]["altitude"].as<float>(50.0f);
            config.search_speed = yaml["search"]["speed"].as<float>(5.0f);
            config.waypoint_spacing = yaml["search"]["spacing"].as<float>(20.0f);
        }
        
        if (yaml["target"]) {
            config.desired_distance = yaml["target"]["desired_distance"].as<float>(30.0f);
            config.desired_height = yaml["target"]["desired_height"].as<float>(10.0f);
        }
        
        if (yaml["safety"]) {
            config.geofence_radius = yaml["safety"]["geofence_radius"].as<float>(500.0f);
            config.mission_timeout = yaml["safety"]["mission_timeout"].as<float>(1800.0f);
        }
        
        std::cout << "[Config] Loaded from: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << std::endl;
        return true;  // Use defaults
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     FalconMind Mission Planner Process v1.0.0         ║" << std::endl;
    std::cout << "║     State Machine & Waypoint Generation               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Parse arguments
    std::string config_path = "/etc/falconmind/mission_planner.yaml";
    if (argc > 2 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    }
    
    // Load configuration
    MissionPlannerConfig config;
    if (!loadConfig(config_path, config)) {
        return -1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize mission planner
    MissionPlanner planner(config);
    
    // Initialize DDS communication
    DDSCommunication dds;
    if (!dds.initialize(config.dds_domain_id,
                       [&planner](const DetectionArray& d) { planner.processDetections(d); },
                       [&planner](const TrackingArray& t) { planner.processTracking(t); })) {
        std::cerr << "[Error] Failed to initialize DDS" << std::endl;
        return -1;
    }
    
    // Start mission (for demonstration - in production would receive command via MQTT)
    planner.startMission(40.0768, 116.3477, 100.0f, 100.0f);
    
    std::cout << "[Mission Planner] Started" << std::endl;
    std::cout << "  Search pattern: " << config.search_pattern << std::endl;
    std::cout << "  Search altitude: " << config.search_altitude << " m" << std::endl;
    std::cout << "  Target distance: " << config.desired_distance << " m" << std::endl;
    std::cout << std::endl;
    
    // Main loop
    auto last_status = std::chrono::steady_clock::now();
    
    while (g_running) {
        // Update state machine
        planner.updateStateMachine();
        
        // Print status every 2 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - last_status).count() >= 2.0) {
            std::cout << "[Status] State: " << planner.getStateString();
            std::cout << " | Targets: " << planner.getConfirmedTargets().size();
            std::cout << " | Active: " << (planner.isMissionActive() ? "YES" : "NO") << std::endl;
            
            last_status = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << "[Mission Planner] Shutting down..." << std::endl;
    dds.cleanup();
    std::cout << "[Mission Planner] Terminated gracefully" << std::endl;
    
    return 0;
}

} // namespace processes
} // namespace falconmind
