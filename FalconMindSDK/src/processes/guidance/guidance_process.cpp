/**
 * @file guidance_process.cpp
 * @brief Guidance Process - IBVS Visual Servoing Controller
 * 
 * This process performs:
 * - Image-Based Visual Servoing (IBVS) control
 * - Target tracking and following
 * - Velocity command generation for flight control
 * - Publishes GuidanceCommand via DDS
 * - Subscribes to DetectionArray and TrackingArray
 * 
 * Architecture:
 *   DetectionArray (DDS) -> IBVS Controller -> GuidanceCommand (DDS)
 *   TrackingArray (DDS)         |              -> Command (MQTT for logging)
 *                               |
 *                               v
 *                         Telemetry (MQTT)
 * 
 * Control Loop (20Hz):
 *   1. Receive detection/tracking data
 *   2. Select best target
 *   3. Compute image error
 *   4. IBVS control law
 *   5. Publish velocity commands
 * 
 * Dependencies:
 *   - Fast DDS
 *   - MQTT (Paho)
 *   - Eigen3 (matrix operations)
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

#include <eigen3/Eigen/Dense>

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <math>
#include <thread>
#include <mutex>
#include <queue>

#include <yaml-cpp/yaml.h>

#include "FalconMindTypes.h"
#include "FalconMindTypesPubSubTypes.h"

using namespace eprosima::fastdds::dds;
using namespace falconmind::dds;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration
// ============================================================================

struct GuidanceConfig {
    // Control parameters
    float desired_distance{30.0f};        // meters
    float distance_tolerance{2.0f};       // +/- tolerance
    float max_velocity{8.0f};            // m/s
    float max_yaw_rate{1.0f};            // rad/s
    
    // PID gains for distance control
    float kp_distance{0.5f};
    float ki_distance{0.1f};
    float kd_distance{0.2f};
    
    // Position control gains
    float kp_position{0.01f};
    
    // Control frequency
    int control_frequency{20};            // Hz
    
    // Target selection
    float min_confidence{0.7f};
    std::vector<std::string> target_classes{"person", "vehicle"};
    
    // DDS settings
    std::string dds_domain_id{"0"};
    std::string detection_topic{"DetectionArray"};
    std::string tracking_topic{"TrackingArray"};
    std::string guidance_topic{"GuidanceCommand"};
    std::string nav_state_topic{"NavigationState"};
    
    // MQTT settings
    std::string mqtt_broker{"tcp://localhost:1883"};
    std::string mqtt_client_id{"falconmind-guidance"};
    std::string mqtt_topic{"falconmind/guidance/command"};
    
    // Safety
    float max_lost_frames{10};           // Max frames without target before abort
    float min_altitude{5.0f};            // Minimum safe altitude
};

// ============================================================================
// IBVS Controller
// ============================================================================

class IBVSController {
public:
    IBVSController(const GuidanceConfig& config)
        : config_(config)
        , integral_error_(0.0f)
        , prev_error_(0.0f)
        , lost_frames_(0)
        , has_target_(false)
    {
    }
    
    struct ControlOutput {
        float vx{0.0f};        // Forward velocity (m/s)
        float vy{0.0f};        // Lateral velocity (m/s)
        float vz{0.0f};        // Vertical velocity (m/s)
        float yaw_rate{0.0f};  // Yaw rate (rad/s)
        bool valid{false};
        std::string error_msg;
    };
    
    ControlOutput compute(const TrackingArray& tracks, 
                         const NavigationState& nav_state,
                         float dt) {
        ControlOutput output;
        
        // Check if we have valid tracks
        if (tracks.tracks().empty()) {
            lost_frames_++;
            if (lost_frames_ > config_.max_lost_frames) {
                output.error_msg = "Target lost for too long";
                has_target_ = false;
                return output;
            }
            // Return hover command
            return ControlOutput{0.0f, 0.0f, 0.0f, 0.0f, true, ""};
        }
        
        // Select best track
        const Track* best_track = selectBestTrack(tracks);
        if (!best_track) {
            output.error_msg = "No suitable target found";
            return output;
        }
        
        lost_frames_ = 0;
        has_target_ = true;
        
        // Extract target info
        float target_distance = best_track->distance_meters();
        float target_confidence = best_track->bbox().confidence();
        
        // Image error (normalized -1 to 1)
        float ex = 0.0f;  // Horizontal error
        float ey = 0.0f;  // Vertical error
        
        // Calculate image center error
        float cx = best_track->bbox().x() + best_track->bbox().width() / 2.0f;
        float cy = best_track->bbox().y() + best_track->bbox().height() / 2.0f;
        
        // Normalize to [-1, 1] range
        ex = (cx - 0.5f) * 2.0f;  // Assuming normalized bbox coordinates
        ey = (cy - 0.5f) * 2.0f;
        
        // Distance error
        float distance_error = target_distance - config_.desired_distance;
        
        // PID control for distance (forward velocity)
        integral_error_ += distance_error * dt;
        integral_error_ = std::clamp(integral_error_, -10.0f, 10.0f);  // Anti-windup
        
        float derivative_error = (distance_error - prev_error_) / dt;
        
        float vx = -(config_.kp_distance * distance_error +
                    config_.ki_distance * integral_error_ +
                    config_.kd_distance * derivative_error);
        
        // Position control (lateral and vertical)
        float vy = -config_.kp_position * ex * target_distance;
        float vz = -config_.kp_position * ey * target_distance;
        
        // Add altitude hold
        float current_altitude = nav_state.position().altitude_relative();
        float altitude_error = current_altitude - 10.0f;  // Hold 10m
        vz += -0.1f * altitude_error;
        
        // Yaw control (point nose at target)
        float yaw_rate = -0.2f * ex;
        
        // Apply saturation limits
        vx = std::clamp(vx, -config_.max_velocity, config_.max_velocity);
        vy = std::clamp(vy, -config_.max_velocity * 0.6f, config_.max_velocity * 0.6f);
        vz = std::clamp(vz, -config_.max_velocity * 0.3f, config_.max_velocity * 0.3f);
        yaw_rate = std::clamp(yaw_rate, -config_.max_yaw_rate, config_.max_yaw_rate);
        
        // Save for next iteration
        prev_error_ = distance_error;
        
        output.vx = vx;
        output.vy = vy;
        output.vz = vz;
        output.yaw_rate = yaw_rate;
        output.valid = true;
        
        return output;
    }
    
    bool hasTarget() const { return has_target_; }
    int getLostFrames() const { return lost_frames_; }
    
    void reset() {
        integral_error_ = 0.0f;
        prev_error_ = 0.0f;
        lost_frames_ = 0;
        has_target_ = false;
    }

private:
    const Track* selectBestTrack(const TrackingArray& tracks) {
        const Track* best = nullptr;
        float best_score = -1.0f;
        
        for (const auto& track : tracks.tracks()) {
            // Skip tentative tracks
            if (track.state() == TRACKING_TENTATIVE) {
                continue;
            }
            
            // Check confidence
            if (track.bbox().confidence() < config_.min_confidence) {
                continue;
            }
            
            // Calculate score
            float score = 0.0f;
            
            // Confidence score
            score += track.bbox().confidence() * 0.4f;
            
            // Distance score (prefer closer targets)
            float dist_score = std::max(0.0f, 1.0f - track.distance_meters() / 100.0f);
            score += dist_score * 0.3f;
            
            // Track quality score
            if (track.state() == TRACKING_CONFIRMED) {
                score += 0.2f;
            }
            
            // Size score (prefer larger targets)
            float size = track.bbox().width() * track.bbox().height();
            score += std::min(size * 10.0f, 0.1f);
            
            if (score > best_score) {
                best_score = score;
                best = &track;
            }
        }
        
        return best;
    }
    
    GuidanceConfig config_;
    float integral_error_;
    float prev_error_;
    int lost_frames_;
    bool has_target_;
};

// ============================================================================
// DDS Listener for Detection/Tracking
// ============================================================================

class TrackingListener : public DataReaderListener {
public:
    void on_data_available(DataReader* reader) override {
        TrackingArray tracking;
        SampleInfo info;
        
        if (reader->take_next_sample(&tracking, &info) == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                std::lock_guard<std::mutex> lock(mutex_);
                latest_tracking_ = tracking;
                has_new_data_ = true;
            }
        }
    }
    
    void on_subscription_matched(DataReader* reader,
                                 const SubscriptionMatchedStatus& info) override {
        if (info.current_count_change == 1) {
            std::cout << "[Guidance] Tracking subscriber matched" << std::endl;
        }
    }
    
    bool getLatestTracking(TrackingArray& tracking) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (has_new_data_) {
            tracking = latest_tracking_;
            has_new_data_ = false;
            return true;
        }
        return false;
    }

private:
    std::mutex mutex_;
    TrackingArray latest_tracking_;
    bool has_new_data_{false};
};

class NavigationListener : public DataReaderListener {
public:
    void on_data_available(DataReader* reader) override {
        NavigationState nav;
        SampleInfo info;
        
        if (reader->take_next_sample(&nav, &info) == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                std::lock_guard<std::mutex> lock(mutex_);
                latest_nav_ = nav;
            }
        }
    }
    
    NavigationState getLatestNav() {
        std::lock_guard<std::mutex> lock(mutex_);
        return latest_nav_;
    }

private:
    std::mutex mutex_;
    NavigationState latest_nav_;
};

// ============================================================================
// Guidance Process Class
// ============================================================================

class GuidanceProcess {
public:
    GuidanceProcess(const GuidanceConfig& config)
        : config_(config)
        , running_(false)
        , participant_(nullptr)
        , subscriber_(nullptr)
        , publisher_(nullptr)
        , tracking_reader_(nullptr)
        , nav_reader_(nullptr)
        , guidance_writer_(nullptr)
        , controller_(config)
        , mqtt_client_(config.mqtt_broker, config.mqtt_client_id)
    {
    }
    
    ~GuidanceProcess() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "[Guidance] Initializing process..." << std::endl;
        
        // Initialize DDS
        if (!initializeDDS()) {
            std::cerr << "[Guidance] Failed to initialize DDS" << std::endl;
            return false;
        }
        
        // Initialize MQTT
        if (!initializeMQTT()) {
            std::cerr << "[Guidance] Failed to initialize MQTT" << std::endl;
            return false;
        }
        
        std::cout << "[Guidance] Process initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        running_ = true;
        
        std::cout << "[Guidance] Starting control loop (" << config_.control_frequency << "Hz)..." << std::endl;
        
        const auto period = std::chrono::milliseconds(1000 / config_.control_frequency);
        auto next_time = std::chrono::steady_clock::now();
        
        uint32_t sequence = 0;
        
        while (running_) {
            next_time += period;
            
            // Run control iteration
            runControlIteration(sequence++);
            
            // Precise timing
            std::this_thread::sleep_until(next_time);
        }
    }
    
    void shutdown() {
        if (!running_) return;
        
        std::cout << "[Guidance] Shutting down..." << std::endl;
        running_ = false;
        
        // Disconnect MQTT
        if (mqtt_client_.is_connected()) {
            mqtt_client_.disconnect()->wait();
        }
        
        // Cleanup DDS
        cleanupDDS();
        
        std::cout << "[Guidance] Shutdown complete" << std::endl;
    }

private:
    bool initializeDDS() {
        // Create participant
        DomainParticipantQos participant_qos;
        participant_qos.name("GuidanceParticipant");
        
        participant_ = DomainParticipantFactory::get_instance()
            ->create_participant(
                static_cast<DomainId_t>(std::stoi(config_.dds_domain_id)),
                participant_qos);
        
        if (!participant_) {
            return false;
        }
        
        // Register types
        tracking_type_.register_type(participant_);
        nav_type_.register_type(participant_);
        guidance_type_.register_type(participant_);
        
        // Create subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        
        // Create publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        
        // Create topics
        tracking_topic_ = participant_->create_topic(
            config_.tracking_topic,
            tracking_type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        nav_topic_ = participant_->create_topic(
            config_.nav_state_topic,
            nav_type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        guidance_topic_ = participant_->create_topic(
            config_.guidance_topic,
            guidance_type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        // Create DataReaders with QoS
        DataReaderQos reader_qos;
        reader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        reader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        reader_qos.history().depth = 1;
        
        tracking_reader_ = subscriber_->create_datareader(
            tracking_topic_, reader_qos, &tracking_listener_);
        
        nav_reader_ = subscriber_->create_datareader(
            nav_topic_, reader_qos, &nav_listener_);
        
        // Create DataWriter with QoS
        DataWriterQos writer_qos;
        writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        writer_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        writer_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        writer_qos.history().depth = 1;
        
        guidance_writer_ = publisher_->create_datawriter(
            guidance_topic_, writer_qos, nullptr);
        
        return true;
    }
    
    bool initializeMQTT() {
        try {
            mqtt::connect_options conn_opts;
            conn_opts.set_keep_alive_interval(20);
            conn_opts.set_clean_session(true);
            conn_opts.set_automatic_reconnect(true);
            
            mqtt_client_.connect(conn_opts)->wait();
            
            std::cout << "[Guidance] MQTT connected to " << config_.mqtt_broker << std::endl;
            return true;
            
        } catch (const mqtt::exception& e) {
            std::cerr << "[Guidance] MQTT connection failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void runControlIteration(uint32_t sequence) {
        // Get latest data
        TrackingArray tracking;
        bool has_tracking = tracking_listener_.getLatestTracking(tracking);
        
        NavigationState nav_state = nav_listener_.getLatestNav();
        
        // Compute control
        float dt = 1.0f / config_.control_frequency;
        auto control = controller_.compute(tracking, nav_state, dt);
        
        // Create guidance command
        GuidanceCommand cmd;
        cmd.uav_id("uav-001");
        cmd.sequence(sequence);
        
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch());
        cmd.timestamp().seconds(ns.count() / 1000000000);
        cmd.timestamp().nanoseconds(ns.count() % 1000000000);
        
        // Set expiration time (100ms)
        cmd.valid_until().seconds(cmd.timestamp().seconds());
        cmd.valid_until().nanoseconds(cmd.timestamp().nanoseconds() + 100000000);
        
        cmd.mode(control.valid ? GUIDANCE_VELOCITY : GUIDANCE_IDLE);
        cmd.tracking_mode(controller_.hasTarget() ? TRACKING_MODE_VELOCITY : TRACKING_MODE_NONE);
        cmd.valid(control.valid);
        cmd.error_message(control.error_msg);
        
        cmd.velocity_x_m_s(control.vx);
        cmd.velocity_y_m_s(control.vy);
        cmd.velocity_z_m_s(control.vz);
        cmd.yaw_rate_dps(control.yaw_rate * 180.0f / M_PI);  // Convert to degrees
        
        cmd.desired_distance_meters(config_.desired_distance);
        
        cmd.max_speed_m_s(config_.max_velocity);
        cmd.max_yaw_rate_dps(config_.max_yaw_rate * 180.0f / M_PI);
        
        // Publish to DDS
        if (guidance_writer_) {
            guidance_writer_->write(&cmd);
        }
        
        // Publish to MQTT (for logging/monitoring)
        publishMQTT(cmd);
        
        // Log every 5 seconds
        static int log_counter = 0;
        if (++log_counter % (config_.control_frequency * 5) == 0) {
            std::cout << "[Guidance] vx=" << control.vx 
                      << " vy=" << control.vy
                      << " vz=" << control.vz
                      << " yaw_rate=" << control.yaw_rate
                      << " valid=" << control.valid << std::endl;
        }
    }
    
    void publishMQTT(const GuidanceCommand& cmd) {
        if (!mqtt_client_.is_connected()) {
            return;
        }
        
        try {
            // Create JSON payload
            std::string payload = "{";
            payload += "\"timestamp\":" + std::to_string(cmd.timestamp().seconds()) + ",";
            payload += "\"sequence\":" + std::to_string(cmd.sequence()) + ",";
            payload += "\"valid\":" + std::string(cmd.valid() ? "true" : "false") + ",";
            payload += "\"mode\":" + std::to_string(static_cast<int>(cmd.mode())) + ",";
            payload += "\"vx\":" + std::to_string(cmd.velocity_x_m_s()) + ",";
            payload += "\"vy\":" + std::to_string(cmd.velocity_y_m_s()) + ",";
            payload += "\"vz\":" + std::to_string(cmd.velocity_z_m_s()) + ",";
            payload += "\"yaw_rate\":" + std::to_string(cmd.yaw_rate_dps());
            payload += "}";
            
            mqtt::message_ptr msg = mqtt::make_message(config_.mqtt_topic, payload);
            msg->set_qos(0);  // Fire and forget for telemetry
            mqtt_client_.publish(msg);
            
        } catch (const mqtt::exception& e) {
            std::cerr << "[Guidance] MQTT publish failed: " << e.what() << std::endl;
        }
    }
    
    void cleanupDDS() {
        if (participant_) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()
                ->delete_participant(participant_);
        }
    }

private:
    GuidanceConfig config_;
    std::atomic<bool> running_;
    
    // DDS
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Publisher* publisher_;
    Topic* tracking_topic_;
    Topic* nav_topic_;
    Topic* guidance_topic_;
    DataReader* tracking_reader_;
    DataReader* nav_reader_;
    DataWriter* guidance_writer_;
    
    TrackingArrayPubSubType tracking_type_;
    NavigationStatePubSubType nav_type_;
    GuidanceCommandPubSubType guidance_type_;
    
    TrackingListener tracking_listener_;
    NavigationListener nav_listener_;
    
    // Controller
    IBVSController controller_;
    
    // MQTT
    mqtt::async_client mqtt_client_;
};

// ============================================================================
// Main Entry Point
// ============================================================================

static std::atomic<bool> g_running{true};
static GuidanceProcess* g_process = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[Guidance] Received signal " << sig << std::endl;
    g_running = false;
    if (g_process) {
        g_process->shutdown();
    }
}

GuidanceConfig loadConfig(const std::string& config_file) {
    GuidanceConfig config;
    
    try {
        YAML::Node yaml = YAML::LoadFile(config_file);
        
        if (yaml["control"]) {
            config.desired_distance = yaml["control"]["desired_distance"].as<float>(config.desired_distance);
            config.max_velocity = yaml["control"]["max_velocity"].as<float>(config.max_velocity);
            config.kp_distance = yaml["control"]["kp"].as<float>(config.kp_distance);
            config.ki_distance = yaml["control"]["ki"].as<float>(config.ki_distance);
            config.kd_distance = yaml["control"]["kd"].as<float>(config.kd_distance);
        }
        
        if (yaml["dds"]) {
            config.dds_domain_id = yaml["dds"]["domain_id"].as<std::string>(config.dds_domain_id);
        }
        
        if (yaml["mqtt"]) {
            config.mqtt_broker = yaml["mqtt"]["broker"].as<std::string>(config.mqtt_broker);
        }
        
        std::cout << "[Guidance] Configuration loaded" << std::endl;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[Guidance] Config load failed: " << e.what() << std::endl;
    }
    
    return config;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "FalconMind Guidance Process v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string config_file = "/etc/falconmind/guidance.yaml";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        }
    }
    
    GuidanceConfig config = loadConfig(config_file);
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    GuidanceProcess process(config);
    g_process = &process;
    
    if (!process.initialize()) {
        std::cerr << "[Guidance] Initialization failed" << std::endl;
        return 1;
    }
    
    process.run();
    
    return 0;
}

} // namespace processes
} // namespace falconmind
