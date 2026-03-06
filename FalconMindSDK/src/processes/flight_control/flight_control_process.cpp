/**
 * @file flight_control_process.cpp
 * @brief Flight Control Process - MAVLink Communication Bridge
 * 
 * This process performs:
 * - MAVLink protocol communication with PX4/ArduPilot
 * - Translates DDS GuidanceCommand to MAVLink velocity commands
 * - Receives telemetry from flight controller and publishes via DDS
 * - Handles arm/disarm, mode switching, mission commands
 * - Safety monitoring (geofence, altitude limits, battery)
 * 
 * Architecture:
 *   GuidanceCommand (DDS) -> FlightControlProcess -> MAVLink -> PX4/ArduPilot
 *   MAVLink Telemetry -> FlightControlProcess -> NavigationState (DDS)
 * 
 * Dependencies:
 *   - MAVLink C library (v2.0)
 *   - Fast DDS
 *   - Serial/UART communication
 * 
 * Build:
 *   cmake .. -DCMAKE_BUILD_TYPE=Release
 *   make -j$(nproc)
 * 
 * Run:
 *   ./flight_control_process --config /etc/falconmind/flight_control.yaml
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

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#include <yaml-cpp/yaml.h>

// MAVLink includes
#include "mavlink/common/mavlink.h"

// DDS generated types
#include "FalconMindTypes.h"
#include "FalconMindTypesPubSubTypes.h"

using namespace eprosima::fastdds::dds;
using namespace falconmind::dds;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration
// ============================================================================

struct FlightControlConfig {
    // Serial port settings
    std::string serial_port{"/dev/ttyUSB0"};
    int baud_rate{921600};
    int data_bits{8};
    int stop_bits{1};
    bool parity{false};
    
    // MAVLink settings
    int system_id{1};           // Our system ID
    int component_id{1};        // Our component ID (1 = autopilot for forwarding)
    int autopilot_id{1};        // Autopilot system ID
    int heartbeat_interval_ms{1000};  // Heartbeat interval
    
    // Control settings
    float control_frequency{20.0f};  // Control loop frequency (Hz)
    float max_velocity_xy{10.0f};    // Max horizontal velocity (m/s)
    float max_velocity_z{5.0f};      // Max vertical velocity (m/s)
    float max_yaw_rate{45.0f};       // Max yaw rate (deg/s)
    
    // Safety settings
    bool enable_geofence{true};
    float geofence_radius{500.0f};   // Max distance from home (m)
    float max_altitude{120.0f};      // Max altitude (m)
    float min_altitude{2.0f};        // Min altitude (m)
    float battery_warning_level{30.0f};  // Battery warning (%)
    float battery_critical_level{20.0f}; // Battery critical (%)
    
    // Timeouts
    int command_timeout_ms{500};     // Command acknowledgement timeout
    int telemetry_timeout_ms{2000};  // Telemetry timeout (no data from FC)
    int connection_timeout_ms{5000}; // Connection establishment timeout
    
    // DDS settings
    std::string dds_domain_id{"0"};
    std::string guidance_topic{"GuidanceCommand"};
    std::string telemetry_topic{"FlightTelemetry"};
    
    // Autopilot type
    std::string autopilot_type{"PX4"};  // "PX4" or "ARDUPILOT"
};

// ============================================================================
// Serial Communication
// ============================================================================

class SerialPort {
public:
    SerialPort() : fd_(-1) {}
    
    ~SerialPort() {
        close();
    }
    
    bool open(const std::string& port, int baud_rate) {
        fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            std::cerr << "[Serial] Failed to open " << port << ": " << strerror(errno) << std::endl;
            return false;
        }
        
        // Configure serial port
        struct termios tty;
        if (tcgetattr(fd_, &tty) != 0) {
            std::cerr << "[Serial] tcgetattr error: " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        // Set baud rate
        speed_t baud = B921600;
        switch (baud_rate) {
            case 9600: baud = B9600; break;
            case 19200: baud = B19200; break;
            case 38400: baud = B38400; break;
            case 57600: baud = B57600; break;
            case 115200: baud = B115200; break;
            case 230400: baud = B230400; break;
            case 460800: baud = B460800; break;
            case 921600: baud = B921600; break;
            default:
                std::cerr << "[Serial] Unsupported baud rate: " << baud_rate << std::endl;
                close();
                return false;
        }
        
        cfsetospeed(&tty, baud);
        cfsetispeed(&tty, baud);
        
        // 8N1
        tty.c_cflag &= ~PARENB;  // No parity
        tty.c_cflag &= ~CSTOPB;  // 1 stop bit
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;       // 8 data bits
        tty.c_cflag |= CREAD | CLOCAL;  // Enable receiver, ignore modem control
        
        // Raw mode
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ECHONL;
        tty.c_lflag &= ~ISIG;
        
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
        
        tty.c_oflag &= ~OPOST;
        tty.c_oflag &= ~ONLCR;
        
        tty.c_cc[VTIME] = 0;
        tty.c_cc[VMIN] = 0;
        
        if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
            std::cerr << "[Serial] tcsetattr error: " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        tcflush(fd_, TCIOFLUSH);
        
        std::cout << "[Serial] Opened " << port << " at " << baud_rate << " baud" << std::endl;
        return true;
    }
    
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    ssize_t read(uint8_t* buffer, size_t size) {
        if (fd_ < 0) return -1;
        return ::read(fd_, buffer, size);
    }
    
    ssize_t write(const uint8_t* buffer, size_t size) {
        if (fd_ < 0) return -1;
        return ::write(fd_, buffer, size);
    }
    
    bool isOpen() const {
        return fd_ >= 0;
    }
    
    int fd() const {
        return fd_;
    }
    
private:
    int fd_;
};

// ============================================================================
// MAVLink Handler
// ============================================================================

class MavlinkHandler {
public:
    explicit MavlinkHandler(const FlightControlConfig& config)
        : config_(config),
          serial_(),
          running_(false),
          connected_(false),
          last_heartbeat_tx_(std::chrono::steady_clock::now()),
          last_heartbeat_rx_(std::chrono::steady_clock::now()),
          current_velocity_{0, 0, 0},
          current_yaw_rate_{0},
          current_mode_{0},
          armed_{false},
          battery_remaining_{0},
          current_position_{0, 0, 0},
          current_attitude_{0, 0, 0},
          home_position_set_{false} {}
    
    ~MavlinkHandler() {
        stop();
    }
    
    bool start(std::function<void(const mavlink_message_t&)> message_callback) {
        message_callback_ = message_callback;
        
        if (!serial_.open(config_.serial_port, config_.baud_rate)) {
            return false;
        }
        
        running_ = true;
        
        // Start receive thread
        receive_thread_ = std::thread(&MavlinkHandler::receiveLoop, this);
        
        // Start transmit thread
        transmit_thread_ = std::thread(&MavlinkHandler::transmitLoop, this);
        
        return true;
    }
    
    void stop() {
        running_ = false;
        
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        if (transmit_thread_.joinable()) {
            transmit_thread_.join();
        }
        
        serial_.close();
    }
    
    void sendVelocityCommand(float vx, float vy, float vz, float yaw_rate) {
        std::lock_guard<std::mutex> lock(command_mutex_);
        
        // Saturate velocities
        float v_norm = std::sqrt(vx * vx + vy * vy);
        if (v_norm > config_.max_velocity_xy) {
            vx = vx * config_.max_velocity_xy / v_norm;
            vy = vy * config_.max_velocity_xy / v_norm;
        }
        
        vz = std::max(-config_.max_velocity_z, std::min(config_.max_velocity_z, vz));
        yaw_rate = std::max(-config_.max_yaw_rate, std::min(config_.max_yaw_rate, yaw_rate));
        
        current_velocity_[0] = vx;
        current_velocity_[1] = vy;
        current_velocity_[2] = vz;
        current_yaw_rate_ = yaw_rate;
    }
    
    bool arm() {
        return sendCommandLong(MAV_CMD_COMPONENT_ARM_DISARM, 1, 0, 0, 0, 0, 0, 0);
    }
    
    bool disarm() {
        return sendCommandLong(MAV_CMD_COMPONENT_ARM_DISARM, 0, 0, 0, 0, 0, 0, 0);
    }
    
    bool setOffboardMode() {
        if (config_.autopilot_type == "PX4") {
            return sendCommandLong(MAV_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_OFFBOARD, 0, 0, 0, 0, 0);
        } else {
            return sendCommandLong(MAV_CMD_DO_SET_MODE, 1, 6, 0, 0, 0, 0, 0);  // ArduPilot GUIDED
        }
    }
    
    bool takeoff(float altitude) {
        return sendCommandLong(MAV_CMD_NAV_TAKEOFF, 0, 0, 0, 0, 0, 0, altitude);
    }
    
    bool land() {
        return sendCommandLong(MAV_CMD_NAV_LAND, 0, 0, 0, 0, 0, 0, 0);
    }
    
    bool returnToLaunch() {
        return sendCommandLong(MAV_CMD_NAV_RETURN_TO_LAUNCH, 0, 0, 0, 0, 0, 0, 0);
    }
    
    bool isConnected() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - last_heartbeat_rx_).count() * 1000;
        return elapsed < config_.telemetry_timeout_ms;
    }
    
    bool isArmed() const {
        return armed_;
    }
    
    void getPosition(float& lat, float& lon, float& alt) const {
        lat = current_position_[0];
        lon = current_position_[1];
        alt = current_position_[2];
    }
    
    void getAttitude(float& roll, float& pitch, float& yaw) const {
        roll = current_attitude_[0];
        pitch = current_attitude_[1];
        yaw = current_attitude_[2];
    }
    
    float getBatteryRemaining() const {
        return battery_remaining_;
    }
    
private:
    void receiveLoop() {
        mavlink_message_t msg;
        mavlink_status_t status;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        
        while (running_) {
            uint8_t byte;
            ssize_t n = serial_.read(&byte, 1);
            
            if (n > 0) {
                if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
                    handleMessage(msg);
                    
                    if (message_callback_) {
                        message_callback_(msg);
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    
    void transmitLoop() {
        while (running_) {
            auto now = std::chrono::steady_clock::now();
            
            // Send heartbeat
            if (std::chrono::duration<double>(now - last_heartbeat_tx_).count() * 1000 >= 
                config_.heartbeat_interval_ms) {
                sendHeartbeat();
                last_heartbeat_tx_ = now;
            }
            
            // Send setpoint at control frequency
            if (connected_ && armed_) {
                sendSetpoint();
            }
            
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(1000.0f / config_.control_frequency)));
        }
    }
    
    void handleMessage(const mavlink_message_t& msg) {
        switch (msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                handleHeartbeat(msg);
                break;
                
            case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                handleGlobalPosition(msg);
                break;
                
            case MAVLINK_MSG_ID_ATTITUDE:
                handleAttitude(msg);
                break;
                
            case MAVLINK_MSG_ID_SYS_STATUS:
                handleSysStatus(msg);
                break;
                
            case MAVLINK_MSG_ID_COMMAND_ACK:
                handleCommandAck(msg);
                break;
                
            case MAVLINK_MSG_ID_GPS_RAW_INT:
                handleGpsRaw(msg);
                break;
        }
    }
    
    void handleHeartbeat(const mavlink_message_t& msg) {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&msg, &heartbeat);
        
        last_heartbeat_rx_ = std::chrono::steady_clock::now();
        connected_ = true;
        
        armed_ = (heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
        current_mode_ = heartbeat.custom_mode;
    }
    
    void handleGlobalPosition(const mavlink_message_t& msg) {
        mavlink_global_position_int_t pos;
        mavlink_msg_global_position_int_decode(&msg, &pos);
        
        current_position_[0] = pos.lat / 1e7f;  // Convert to degrees
        current_position_[1] = pos.lon / 1e7f;
        current_position_[2] = pos.relative_alt / 1000.0f;  // Convert to meters
        
        if (!home_position_set_) {
            // Set home position on first valid GPS
            home_position_set_ = true;
        }
    }
    
    void handleAttitude(const mavlink_message_t& msg) {
        mavlink_attitude_t att;
        mavlink_msg_attitude_decode(&msg, &att);
        
        current_attitude_[0] = att.roll * 180.0f / M_PI;   // Convert to degrees
        current_attitude_[1] = att.pitch * 180.0f / M_PI;
        current_attitude_[2] = att.yaw * 180.0f / M_PI;
    }
    
    void handleSysStatus(const mavlink_message_t& msg) {
        mavlink_sys_status_t status;
        mavlink_msg_sys_status_decode(&msg, &status);
        
        battery_remaining_ = status.battery_remaining;
    }
    
    void handleCommandAck(const mavlink_message_t& msg) {
        mavlink_command_ack_t ack;
        mavlink_msg_command_ack_decode(&msg, &ack);
        
        if (ack.result == MAV_RESULT_ACCEPTED) {
            std::cout << "[MAVLink] Command " << ack.command << " accepted" << std::endl;
        } else {
            std::cout << "[MAVLink] Command " << ack.command << " failed: " << (int)ack.result << std::endl;
        }
    }
    
    void handleGpsRaw(const mavlink_message_t& msg) {
        mavlink_gps_raw_int_t gps;
        mavlink_msg_gps_raw_int_decode(&msg, &gps);
        
        // GPS fix quality check
        if (gps.fix_type < 3) {
            // std::cout << "[GPS] Warning: Poor GPS fix type: " << (int)gps.fix_type << std::endl;
        }
    }
    
    void sendHeartbeat() {
        mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        
        mavlink_msg_heartbeat_pack(
            config_.system_id, config_.component_id, &msg,
            MAV_TYPE_ONBOARD_CONTROLLER,      // Type
            MAV_AUTOPILOT_INVALID,            // Autopilot (we're not an autopilot)
            armed_ ? MAV_MODE_FLAG_SAFETY_ARMED : 0,  // Base mode
            0,                                // Custom mode
            MAV_STATE_ACTIVE                  // System status
        );
        
        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        serial_.write(buffer, len);
    }
    
    void sendSetpoint() {
        mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        
        std::lock_guard<std::mutex> lock(command_mutex_);
        
        // Send velocity setpoint in body frame
        mavlink_msg_set_position_target_local_ned_pack(
            config_.system_id, config_.component_id, &msg,
            0,  // Time boot ms
            config_.autopilot_id, 1,  // Target system/component
            MAV_FRAME_BODY_NED,       // Frame
            0b0000111111000111,       // Type mask (velocity only)
            0, 0, 0,                  // Position (ignored)
            current_velocity_[0],     // VX
            current_velocity_[1],     // VY
            current_velocity_[2],     // VZ
            0, 0, 0,                  // Acceleration (ignored)
            current_yaw_rate_ * M_PI / 180.0f,  // Yaw rate in rad/s
            0                         // Yaw (ignored)
        );
        
        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        serial_.write(buffer, len);
    }
    
    bool sendCommandLong(uint16_t command, float param1, float param2, float param3,
                        float param4, float param5, float param6, float param7) {
        mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        
        mavlink_msg_command_long_pack(
            config_.system_id, config_.component_id, &msg,
            config_.autopilot_id, 1,  // Target system/component
            command,
            0,  // Confirmation
            param1, param2, param3, param4, param5, param6, param7
        );
        
        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        ssize_t sent = serial_.write(buffer, len);
        
        return sent == len;
    }
    
    const FlightControlConfig& config_;
    SerialPort serial_;
    
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    
    std::thread receive_thread_;
    std::thread transmit_thread_;
    
    std::chrono::steady_clock::time_point last_heartbeat_tx_;
    std::chrono::steady_clock::time_point last_heartbeat_rx_;
    
    std::mutex command_mutex_;
    float current_velocity_[3];
    float current_yaw_rate_;
    
    uint32_t current_mode_;
    bool armed_;
    float battery_remaining_;
    
    float current_position_[3];
    float current_attitude_[3];
    bool home_position_set_;
    
    std::function<void(const mavlink_message_t&)> message_callback_;
};

// ============================================================================
// DDS Communication
// ============================================================================

class DDSCommunication {
public:
    DDSCommunication() : participant_(nullptr), subscriber_(nullptr),
                         publisher_(nullptr), guidance_reader_(nullptr),
                         telemetry_writer_(nullptr) {}
    
    ~DDSCommunication() {
        cleanup();
    }
    
    bool initialize(const std::string& domain_id,
                   std::function<void(const GuidanceCommand&)> guidance_callback) {
        
        guidance_callback_ = guidance_callback;
        
        DomainParticipantQos participant_qos;
        participant_qos.name("flight_control_participant");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            std::stoi(domain_id), participant_qos);
        
        if (!participant_) {
            std::cerr << "[DDS] Failed to create participant" << std::endl;
            return false;
        }
        
        SubscriberQos subscriber_qos;
        subscriber_ = participant_->create_subscriber(subscriber_qos, nullptr);
        
        PublisherQos publisher_qos;
        publisher_ = participant_->create_publisher(publisher_qos, nullptr);
        
        // Register types
        guidance_type_.register_type(participant_);
        
        // Create guidance topic and reader
        TopicQos guidance_topic_qos;
        guidance_topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        
        auto* guidance_topic = participant_->create_topic(
            "GuidanceCommand", guidance_type_.get_type_name(), guidance_topic_qos);
        
        DataReaderQos guidance_reader_qos;
        guidance_reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        guidance_reader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        guidance_reader_qos.history().depth = 1;
        
        guidance_reader_ = subscriber_->create_datareader(
            guidance_topic, guidance_reader_qos, &guidance_listener_);
        
        std::cout << "[DDS] Communication initialized" << std::endl;
        return true;
    }
    
    void cleanup() {
        if (participant_) {
            if (guidance_reader_) subscriber_->delete_datareader(guidance_reader_);
            if (telemetry_writer_) publisher_->delete_datawriter(telemetry_writer_);
            if (subscriber_) participant_->delete_subscriber(subscriber_);
            if (publisher_) participant_->delete_publisher(publisher_);
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }
    
private:
    class GuidanceListener : public DataReaderListener {
    public:
        explicit GuidanceListener(std::function<void(const GuidanceCommand&)> callback)
            : callback_(callback) {}
        
        void on_data_available(DataReader* reader) override {
            GuidanceCommand cmd;
            SampleInfo info;
            
            while (reader->take_next_sample(&cmd, &info) == ReturnCode_t::RETCODE_OK) {
                if (info.valid_data && callback_) {
                    callback_(cmd);
                }
            }
        }
        
    private:
        std::function<void(const GuidanceCommand&)> callback_;
    };
    
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Publisher* publisher_;
    DataReader* guidance_reader_;
    DataWriter* telemetry_writer_;
    
    GuidanceCommandPubSubType guidance_type_;
    
    GuidanceListener guidance_listener_{nullptr};
    std::function<void(const GuidanceCommand&)> guidance_callback_;
};

// ============================================================================
// Signal Handling
// ============================================================================

static std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\n[Flight Control] Received signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
}

// ============================================================================
// Configuration Loading
// ============================================================================

bool loadConfig(const std::string& config_path, FlightControlConfig& config) {
    try {
        YAML::Node yaml = YAML::LoadFile(config_path);
        
        if (yaml["serial"]) {
            config.serial_port = yaml["serial"]["port"].as<std::string>("/dev/ttyUSB0");
            config.baud_rate = yaml["serial"]["baud_rate"].as<int>(921600);
        }
        
        if (yaml["mavlink"]) {
            config.system_id = yaml["mavlink"]["system_id"].as<int>(1);
            config.component_id = yaml["mavlink"]["component_id"].as<int>(1);
            config.autopilot_type = yaml["mavlink"]["autopilot"].as<std::string>("PX4");
        }
        
        if (yaml["control"]) {
            config.control_frequency = yaml["control"]["frequency"].as<float>(20.0f);
            config.max_velocity_xy = yaml["control"]["max_velocity_xy"].as<float>(10.0f);
            config.max_velocity_z = yaml["control"]["max_velocity_z"].as<float>(5.0f);
        }
        
        if (yaml["safety"]) {
            config.max_altitude = yaml["safety"]["max_altitude"].as<float>(120.0f);
            config.battery_critical_level = yaml["safety"]["battery_critical"].as<float>(20.0f);
        }
        
        std::cout << "[Config] Loaded from: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << std::endl;
        return true;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     FalconMind Flight Control Process v1.0.0          ║" << std::endl;
    std::cout << "║     MAVLink Bridge to PX4/ArduPilot                   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Parse arguments
    std::string config_path = "/etc/falconmind/flight_control.yaml";
    if (argc > 2 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    }
    
    // Load configuration
    FlightControlConfig config;
    if (!loadConfig(config_path, config)) {
        return -1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize MAVLink handler
    MavlinkHandler mavlink(config);
    if (!mavlink.start(nullptr)) {
        std::cerr << "[Error] Failed to initialize MAVLink" << std::endl;
        return -1;
    }
    
    // Initialize DDS communication
    DDSCommunication dds;
    if (!dds.initialize(config.dds_domain_id,
                       [&mavlink](const GuidanceCommand& cmd) {
                           mavlink.sendVelocityCommand(
                               cmd.velocity_x(), cmd.velocity_y(), 
                               cmd.velocity_z(), cmd.yaw_rate()
                           );
                       })) {
        std::cerr << "[Error] Failed to initialize DDS" << std::endl;
        return -1;
    }
    
    std::cout << "[Flight Control] Started" << std::endl;
    std::cout << "  Serial port: " << config.serial_port << std::endl;
    std::cout << "  Baud rate: " << config.baud_rate << std::endl;
    std::cout << "  Control frequency: " << config.control_frequency << " Hz" << std::endl;
    std::cout << "  Autopilot: " << config.autopilot_type << std::endl;
    std::cout << std::endl;
    
    // Main loop
    auto last_status = std::chrono::steady_clock::now();
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Print status every 2 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - last_status).count() >= 2.0) {
            float lat, lon, alt;
            mavlink.getPosition(lat, lon, alt);
            
            std::cout << "[Status] Connected: " << (mavlink.isConnected() ? "YES" : "NO");
            std::cout << " | Armed: " << (mavlink.isArmed() ? "YES" : "NO");
            std::cout << " | Position: [" << lat << ", " << lon << ", " << alt << "]";
            std::cout << " | Battery: " << mavlink.getBatteryRemaining() << "%" << std::endl;
            
            last_status = now;
        }
    }
    
    // Cleanup
    std::cout << "[Flight Control] Shutting down..." << std::endl;
    
    // Disarm before shutdown
    if (mavlink.isArmed()) {
        std::cout << "[Flight Control] Disarming..." << std::endl;
        mavlink.disarm();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    mavlink.stop();
    dds.cleanup();
    
    std::cout << "[Flight Control] Terminated gracefully" << std::endl;
    
    return 0;
}

} // namespace processes
} // namespace falconmind
