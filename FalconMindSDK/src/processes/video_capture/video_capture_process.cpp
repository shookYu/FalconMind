/**
 * @file video_capture_process.cpp
 * @brief Video Capture Process - Production GStreamer RTSP Pipeline
 * 
 * This process handles:
 * - Camera acquisition (V4L2, MIPI CSI)
 * - Hardware-accelerated encoding (RK3588 MPP/VPU)
 * - RTSP streaming to MediaTx
 * - Frame metadata publishing via DDS
 * 
 * Architecture:
 *   Camera Source -> GStreamer Pipeline -> RTSP Sink
 *                          |
 *                          v
 *                    DDS Publisher (frame metadata)
 * 
 * Dependencies:
 *   - GStreamer 1.20+
 *   - gstreamer1.0-rtsp-server
 *   - RK3588 MPP/VPU libraries (for hardware encoding)
 *   - Fast DDS
 * 
 * Build:
 *   mkdir build && cd build
 *   cmake .. -DFALCONMIND_PLATFORM=RK3588
 *   make -j$(nproc)
 * 
 * Run:
 *   ./video_capture_process --config /etc/falconmind/video_capture.yaml
 * 
 * @author FalconMind Engineering Team
 * @version 1.0.0
 */

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <fstream>

#include <yaml-cpp/yaml.h>

// DDS generated types
#include "FalconMindTypes.h"
#include "FalconMindTypesPubSubTypes.h"

using namespace eprosima::fastdds::dds;
using namespace falconmind::dds;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration Structure
// ============================================================================

struct VideoCaptureConfig {
    // Camera settings
    std::string camera_source;      // "/dev/video0" or "mipi"
    int camera_width{1920};
    int camera_height{1080};
    int camera_fps{30};
    std::string camera_format{"NV12"};  // NV12, YUV422, RGB
    
    // Encoding settings
    std::string encoder{"x264enc"};  // x264enc, mpph264enc (RK3588), vaapih264enc
    int video_bitrate{4000};         // kbps
    int video_quality{4};            // 0-9, lower is better
    
    // RTSP settings
    std::string rtsp_mount{"/live/camera"};
    int rtsp_port{8554};
    std::string rtsp_factory_path{"/live/camera"};
    
    // DDS settings
    std::string dds_domain_id{"0"};
    std::string dds_topic_name{"VideoFrameMetadata"};
    
    // Pipeline settings
    int pipeline_buffer_size{10};
    bool enable_overlay{false};
    std::string overlay_text{"FalconMind Camera"};
    
    // Hardware acceleration
    bool enable_hw_accel{true};
    std::string hw_accel_type{"mpp"};  // mpp (RK3588), vaapi, none
    
    // Logging
    std::string log_level{"info"};
    std::string log_file{"/var/log/falconmind/video_capture.log"};
};

// ============================================================================
// Video Capture Process Class
// ============================================================================

class VideoCaptureProcess {
public:
    VideoCaptureProcess(const VideoCaptureConfig& config)
        : config_(config)
        , running_(false)
        , pipeline_(nullptr)
        , rtsp_server_(nullptr)
        , factory_(nullptr)
        , participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
    {
    }
    
    ~VideoCaptureProcess() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "[VideoCapture] Initializing process..." << std::endl;
        
        // Initialize GStreamer
        if (!initializeGStreamer()) {
            std::cerr << "[VideoCapture] Failed to initialize GStreamer" << std::endl;
            return false;
        }
        
        // Initialize DDS
        if (!initializeDDS()) {
            std::cerr << "[VideoCapture] Failed to initialize DDS" << std::endl;
            return false;
        }
        
        // Initialize RTSP Server
        if (!initializeRTSPServer()) {
            std::cerr << "[VideoCapture] Failed to initialize RTSP server" << std::endl;
            return false;
        }
        
        std::cout << "[VideoCapture] Process initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        running_ = true;
        
        std::cout << "[VideoCapture] Starting main loop..." << std::endl;
        std::cout << "[VideoCapture] RTSP URL: rtsp://localhost:" << config_.rtsp_port << config_.rtsp_mount << std::endl;
        
        // Start DDS publish thread
        dds_thread_ = std::thread(&VideoCaptureProcess::ddsPublishLoop, this);
        
        // Run GStreamer main loop
        g_main_loop_run(loop_);
    }
    
    void shutdown() {
        if (!running_) return;
        
        std::cout << "[VideoCapture] Shutting down..." << std::endl;
        running_ = false;
        
        // Stop DDS thread
        if (dds_thread_.joinable()) {
            dds_thread_.join();
        }
        
        // Stop GStreamer
        if (loop_) {
            g_main_loop_quit(loop_);
        }
        
        // Cleanup DDS
        cleanupDDS();
        
        // Cleanup GStreamer
        cleanupGStreamer();
        
        std::cout << "[VideoCapture] Shutdown complete" << std::endl;
    }

private:
    // ========================================================================
    // GStreamer Initialization
    // ========================================================================
    
    bool initializeGStreamer() {
        GError* error = nullptr;
        
        // Initialize GStreamer
        if (!gst_init_check(nullptr, nullptr, &error)) {
            std::cerr << "[VideoCapture] GStreamer initialization failed: " 
                      << (error ? error->message : "unknown error") << std::endl;
            if (error) g_error_free(error);
            return false;
        }
        
        // Set log level
        gst_debug_set_default_threshold(
            config_.log_level == "debug" ? GST_LEVEL_DEBUG :
            config_.log_level == "info" ? GST_LEVEL_INFO :
            config_.log_level == "warn" ? GST_LEVEL_WARNING :
            GST_LEVEL_ERROR
        );
        
        // Create main loop
        loop_ = g_main_loop_new(nullptr, FALSE);
        if (!loop_) {
            std::cerr << "[VideoCapture] Failed to create GMainLoop" << std::endl;
            return false;
        }
        
        return true;
    }
    
    // ========================================================================
    // RTSP Server Initialization
    // ========================================================================
    
    bool initializeRTSPServer() {
        // Create RTSP server
        rtsp_server_ = gst_rtsp_server_new();
        if (!rtsp_server_) {
            std::cerr << "[VideoCapture] Failed to create RTSP server" << std::endl;
            return false;
        }
        
        // Set port
        gst_rtsp_server_set_service(rtsp_server_, 
            std::to_string(config_.rtsp_port).c_str());
        
        // Create media factory
        factory_ = gst_rtsp_media_factory_new();
        if (!factory_) {
            std::cerr << "[VideoCapture] Failed to create media factory" << std::endl;
            return false;
        }
        
        // Build pipeline string
        std::string pipeline_str = buildPipelineString();
        std::cout << "[VideoCapture] Pipeline: " << pipeline_str << std::endl;
        
        // Set pipeline on factory
        gst_rtsp_media_factory_set_launch(factory_, pipeline_str.c_str());
        gst_rtsp_media_factory_set_shared(factory_, TRUE);
        
        // Get mount points
        GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(rtsp_server_);
        
        // Attach factory to path
        gst_rtsp_mount_points_add_factory(mounts, config_.rtsp_mount.c_str(), factory_);
        g_object_unref(mounts);
        
        // Attach server to main context
        if (gst_rtsp_server_attach(rtsp_server_, nullptr) == 0) {
            std::cerr << "[VideoCapture] Failed to attach RTSP server" << std::endl;
            return false;
        }
        
        return true;
    }
    
    std::string buildPipelineString() {
        std::ostringstream oss;
        
        // Camera source
        if (config_.camera_source.find("/dev/video") != std::string::npos) {
            // V4L2 source
            oss << "v4l2src device=" <> config_.camera_source
                << " io-mode=2 ! ";
        } else if (config_.camera_source == "mipi") {
            // MIPI CSI source (RK3588)
            oss << "v4l2src device=/dev/video0 io-mode=mmap ! ";
        } else if (config_.camera_source == "test") {
            // Test source for development
            oss << "videotestsrc pattern=ball is-live=true ! ";
        }
        
        // Video format
        oss << "video/x-raw,format=" << config_.camera_format
            << ",width=" << config_.camera_width
            << ",height=" << config_.camera_height
            << ",framerate=" <> config_.camera_fps << "/1 ! ";
        
        // Hardware acceleration check
        bool use_hw_accel = config_.enable_hw_accel && config_.hw_accel_type != "none";
        
        if (use_hw_accel) {
            if (config_.hw_accel_type == "mpp") {
                // Rockchip MPP hardware encoder
                oss << "mpph264enc "
                    << "rc-mode=vbr "
                    << "bps=" << (config_.video_bitrate * 1000) << " "
                    << "gop=" <> config_.camera_fps << " "
                    << "level=41 ! ";
            } else if (config_.hw_accel_type == "vaapi") {
                // Intel VAAPI
                oss << "vaapih264enc "
                    << "rate-control=vbr "
                    << "bitrate=" <> config_.video_bitrate << " ! ";
            }
        } else {
            // Software encoder (x264)
            oss << "videoconvert ! "
                <> "x264enc "
                << "tune=zerolatency "
                << "speed-preset=ultrafast "
                << "bitrate=" <> config_.video_bitrate << " "
                << "key-int-max=" <> config_.camera_fps << " ! ";
        }
        
        // H264 parse and payload
        oss << "h264parse ! "
            << "rtph264pay name=pay0 pt=96 config-interval=1 ! "
            << "appsink name=meta_sink";
        
        return oss.str();
    }
    
    // ========================================================================
    // DDS Initialization
    // ========================================================================
    
    bool initializeDDS() {
        // Create participant
        DomainParticipantQos participant_qos;
        participant_qos.name("VideoCaptureParticipant");
        
        // Load QoS profile
        participant_qos = DomainParticipantFactory::get_instance()
            ->get_default_participant_qos();
        
        participant_ = DomainParticipantFactory::get_instance()
            ->create_participant(
                static_cast<DomainId_t>(std::stoi(config_.dds_domain_id)),
                participant_qos);
        
        if (!participant_) {
            std::cerr << "[VideoCapture] Failed to create DDS participant" << std::endl;
            return false;
        }
        
        // Register type
        type_.register_type(participant_);
        
        // Create topic
        topic_ = participant_->create_topic(
            config_.dds_topic_name,
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        if (!topic_) {
            std::cerr << "[VideoCapture] Failed to create DDS topic" << std::endl;
            return false;
        }
        
        // Create publisher with QoS
        PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
        publisher_ = participant_->create_publisher(publisher_qos, nullptr);
        
        if (!publisher_) {
            std::cerr << "[VideoCapture] Failed to create DDS publisher" << std::endl;
            return false;
        }
        
        // Create DataWriter with QoS optimized for video metadata
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        
        // Best-effort reliability for low latency
        writer_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        
        // Volatile durability (don't persist for late joiners)
        writer_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        
        // Keep only latest sample
        writer_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        writer_qos.history().depth = 1;
        
        // Deadline for 30fps
        writer_qos.deadline().period.seconds = 0;
        writer_qos.deadline().period.nanosec = 33333333;  // ~33ms
        
        writer_ = publisher_->create_datawriter(topic_, writer_qos, &listener_);
        
        if (!writer_) {
            std::cerr << "[VideoCapture] Failed to create DDS datawriter" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void cleanupDDS() {
        if (participant_) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()
                ->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
    
    void cleanupGStreamer() {
        if (rtsp_server_) {
            g_object_unref(rtsp_server_);
            rtsp_server_ = nullptr;
        }
        
        if (loop_) {
            g_main_loop_unref(loop_);
            loop_ = nullptr;
        }
        
        gst_deinit();
    }
    
    // ========================================================================
    // DDS Publish Loop
    // ========================================================================
    
    void ddsPublishLoop() {
        std::cout << "[VideoCapture] DDS publish thread started" << std::endl;
        
        uint32_t sequence = 0;
        
        while (running_) {
            // Create frame metadata
            VideoFrameMetadata metadata;
            
            // Populate metadata
            metadata.stream_id("camera");
            metadata.rtsp_url(
                "rtsp://localhost:" + 
                std::to_string(config_.rtsp_port) + 
                config_.rtsp_mount);
            metadata.sequence(sequence++);
            
            auto now = std::chrono::system_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch());
            metadata.timestamp().seconds(ns.count() / 1000000000);
            metadata.timestamp().nanoseconds(ns.count() % 1000000000);
            
            metadata.width(config_.camera_width);
            metadata.height(config_.camera_height);
            metadata.fps(static_cast<float>(config_.camera_fps));
            metadata.encoding("h264");
            metadata.bitrate_kbps(config_.video_bitrate);
            
            // Publish
            if (writer_) {
                writer_->write(&metadata);
            }
            
            // 30Hz metadata publish
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
        
        std::cout << "[VideoCapture] DDS publish thread stopped" << std::endl;
    }

private:
    VideoCaptureConfig config_;
    std::atomic<bool> running_;
    
    // GStreamer
    GMainLoop* loop_;
    GstElement* pipeline_;
    GstRTSPServer* rtsp_server_;
    GstRTSPMediaFactory* factory_;
    
    // DDS
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    VideoFrameMetadataPubSubType type_;
    
    class VideoFrameListener : public DataWriterListener {
    public:
        void on_publication_matched(DataWriter* writer,
                                    const PublicationMatchedStatus& info) override {
            if (info.current_count_change == 1) {
                std::cout << "[VideoCapture] DDS subscriber matched" << std::endl;
            } else if (info.current_count_change == -1) {
                std::cout << "[VideoCapture] DDS subscriber unmatched" << std::endl;
            }
        }
    };
    
    VideoFrameListener listener_;
    std::thread dds_thread_;
};

// ============================================================================
// Signal Handling
// ============================================================================

static std::atomic<bool> g_running{true};
static VideoCaptureProcess* g_process = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[VideoCapture] Received signal " << sig << std::endl;
    g_running = false;
    if (g_process) {
        g_process->shutdown();
    }
}

// ============================================================================
// Configuration Loading
// ============================================================================

VideoCaptureConfig loadConfig(const std::string& config_file) {
    VideoCaptureConfig config;
    
    try {
        YAML::Node yaml = YAML::LoadFile(config_file);
        
        // Camera settings
        if (yaml["camera"]) {
            config.camera_source = yaml["camera"]["source"].as<std::string>(config.camera_source);
            config.camera_width = yaml["camera"]["width"].as<int>(config.camera_width);
            config.camera_height = yaml["camera"]["height"].as<int>(config.camera_height);
            config.camera_fps = yaml["camera"]["fps"].as<int>(config.camera_fps);
            config.camera_format = yaml["camera"]["format"].as<std::string>(config.camera_format);
        }
        
        // Encoding
        if (yaml["encoding"]) {
            config.encoder = yaml["encoding"]["encoder"].as<std::string>(config.encoder);
            config.video_bitrate = yaml["encoding"]["bitrate"].as<int>(config.video_bitrate);
            config.video_quality = yaml["encoding"]["quality"].as<int>(config.video_quality);
        }
        
        // RTSP
        if (yaml["rtsp"]) {
            config.rtsp_mount = yaml["rtsp"]["mount"].as<std::string>(config.rtsp_mount);
            config.rtsp_port = yaml["rtsp"]["port"].as<int>(config.rtsp_port);
        }
        
        // DDS
        if (yaml["dds"]) {
            config.dds_domain_id = yaml["dds"]["domain_id"].as<std::string>(config.dds_domain_id);
            config.dds_topic_name = yaml["dds"]["topic"].as<std::string>(config.dds_topic_name);
        }
        
        // Hardware acceleration
        if (yaml["hardware"]) {
            config.enable_hw_accel = yaml["hardware"]["enabled"].as<bool>(config.enable_hw_accel);
            config.hw_accel_type = yaml["hardware"]["type"].as<std::string>(config.hw_accel_type);
        }
        
        // Logging
        if (yaml["logging"]) {
            config.log_level = yaml["logging"]["level"].as<std::string>(config.log_level);
            config.log_file = yaml["logging"]["file"].as<std::string>(config.log_file);
        }
        
        std::cout << "[VideoCapture] Configuration loaded from: " << config_file << std::endl;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[VideoCapture] Failed to load config: " << e.what() << std::endl;
        std::cerr << "[VideoCapture] Using default configuration" << std::endl;
    }
    
    return config;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "FalconMind Video Capture Process v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Parse command line
    std::string config_file = "/etc/falconmind/video_capture.yaml";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -c, --config FILE    Configuration file (default: "
                      << config_file << ")\n"
                      << "  -h, --help          Show this help\n";
            return 0;
        }
    }
    
    // Load configuration
    VideoCaptureConfig config = loadConfig(config_file);
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    
    // Create and run process
    VideoCaptureProcess process(config);
    g_process = &process;
    
    if (!process.initialize()) {
        std::cerr << "[VideoCapture] Failed to initialize process" << std::endl;
        return 1;
    }
    
    process.run();
    
    std::cout << "[VideoCapture] Process exited normally" << std::endl;
    return 0;
}

} // namespace processes
} // namespace falconmind
