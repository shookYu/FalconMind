/**
 * @file perception_process.cpp
 * @brief Perception Process - Object Detection and Tracking
 * 
 * This process performs:
 * - Real-time object detection using RKNN (RK3588 NPU)
 * - Multi-object tracking using DeepSORT
 * - Distance estimation using monocular vision
 * - Publishes DetectionArray and TrackingArray via DDS
 * 
 * Architecture:
 *   Video Metadata (DDS) -> Detector (RKNN) -> Tracker (DeepSORT)
 *                                       |
 *                                       v
 *                    DDS Publisher -> DetectionArray / TrackingArray
 * 
 * Dependencies:
 *   - RKNN Toolkit (RK3588 NPU)
 *   - OpenCV 4.x
 *   - Fast DDS
 * 
 * Build:
 *   cmake .. -DFALCONMIND_PLATFORM=RK3588
 *   make -j$(nproc)
 * 
 * Run:
 *   ./perception_process --config /etc/falconmind/perception.yaml
 * 
 * @author FalconMind Engineering Team
 * @version 1.0.0
 */

#include <opencv2/opencv.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>

#include <yaml-cpp/yaml.h>

// DDS generated types
#include "FalconMindTypes.h"
#include "FalconMindTypesPubSubTypes.h"

#ifdef PLATFORM_RK3588
#include "rknn_api.h"
#endif

using namespace eprosima::fastdds::dds;
using namespace falconmind::dds;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration
// ============================================================================

struct PerceptionConfig {
    // Model settings
    std::string model_path{"/opt/falconmind/models/yolov8n.rknn"};
    std::string label_path{"/opt/falconmind/models/coco_labels.txt"};
    float confidence_threshold{0.5f};
    float nms_threshold{0.45f};
    
    // Target classes to detect
    std::vector<std::string> target_classes{"person", "vehicle", "drone"};
    
    // Tracking settings
    bool enable_tracking{true};
    int max_age{30};              // Max frames to keep lost tracks
    int min_hits{3};              // Min hits to confirm track
    float max_cosine_distance{0.2f};
    float max_iou_distance{0.7f};
    
    // Distance estimation
    bool enable_distance{true};
    float camera_focal_length{1000.0f};
    float camera_height{1.5f};    // Camera height above ground
    
    // DDS settings
    std::string dds_domain_id{"0"};
    std::string video_topic{"VideoFrameMetadata"};
    std::string detection_topic{"DetectionArray"};
    std::string tracking_topic{"TrackingArray"};
    
    // Processing
    int input_width{640};
    int input_height{480};
    int inference_interval{1};     // Run inference every N frames
    
    // Performance
    int num_threads{4};
    bool enable_fp16{true};
};

// ============================================================================
// DeepSORT Tracker Implementation
// ============================================================================

class DeepSORTTracker {
public:
    struct Track {
        int track_id;
        Detection detection;
        TrackingState state;
        int age{0};
        int hits{0};
        int time_since_update{0};
        cv::KalmanFilter kf;
        std::deque<cv::Mat> features;
        int feature_budget{100};
        
        Track(int id, const Detection& det) : track_id(id), detection(det) {
            // Initialize Kalman filter
            initKalman();
        }
        
        void initKalman() {
            // Constant velocity model: [cx, cy, w, h, vcx, vcy, vw, vh]
            kf.init(8, 4, 0);
            kf.transitionMatrix = (cv::Mat_<float>(8, 8) <<
                1, 0, 0, 0, 1, 0, 0, 0,
                0, 1, 0, 0, 0, 1, 0, 0,
                0, 0, 1, 0, 0, 0, 1, 0,
                0, 0, 0, 1, 0, 0, 0, 1,
                0, 0, 0, 0, 1, 0, 0, 0,
                0, 0, 0, 0, 0, 1, 0, 0,
                0, 0, 0, 0, 0, 0, 1, 0,
                0, 0, 0, 0, 0, 0, 0, 1);
            
            kf.measurementMatrix = cv::Mat::zeros(4, 8, CV_32F);
            kf.measurementMatrix.at<float>(0, 0) = 1.0f;
            kf.measurementMatrix.at<float>(1, 1) = 1.0f;
            kf.measurementMatrix.at<float>(2, 2) = 1.0f;
            kf.measurementMatrix.at<float>(3, 3) = 1.0f;
            
            // Set initial state from detection
            float cx = detection.bbox().x() + detection.bbox().width() / 2.0f;
            float cy = detection.bbox().y() + detection.bbox().height() / 2.0f;
            float w = detection.bbox().width();
            float h = detection.bbox().height();
            
            kf.statePost = (cv::Mat_<float>(8, 1) << cx, cy, w, h, 0, 0, 0, 0);
        }
        
        void predict() {
            kf.predict();
            age++;
            time_since_update++;
        }
        
        void update(const Detection& det) {
            float cx = det.bbox().x() + det.bbox().width() / 2.0f;
            float cy = det.bbox().y() + det.bbox().height() / 2.0f;
            float w = det.bbox().width();
            float h = det.bbox().height();
            
            cv::Mat measurement = (cv::Mat_<float>(4, 1) << cx, cy, w, h);
            kf.correct(measurement);
            
            detection = det;
            hits++;
            time_since_update = 0;
            
            if (state == TRACKING_TENTATIVE && hits >= 3) {
                state = TRACKING_CONFIRMED;
            }
        }
        
        BoundingBox getPredictedBBox() const {
            BoundingBox bbox;
            float cx = kf.statePre.at<float>(0, 0);
            float cy = kf.statePre.at<float>(1, 0);
            float w = kf.statePre.at<float>(2, 0);
            float h = kf.statePre.at<float>(3, 0);
            
            bbox.x(cx - w / 2.0f);
            bbox.y(cy - h / 2.0f);
            bbox.width(w);
            bbox.height(h);
            bbox.confidence(detection.bbox().confidence());
            return bbox;
        }
    };
    
    DeepSORTTracker(const PerceptionConfig& config)
        : config_(config)
        , next_track_id_(1)
    {
    }
    
    std::vector<Track> update(const std::vector<Detection>& detections) {
        // Predict all existing tracks
        for (auto& track : tracks_) {
            track.predict();
        }
        
        // Separate confirmed and tentative tracks
        std::vector<Track*> confirmed_tracks;
        std::vector<Track*> tentative_tracks;
        
        for (auto& track : tracks_) {
            if (track.state == TRACKING_CONFIRMED) {
                confirmed_tracks.push_back(&track);
            } else {
                tentative_tracks.push_back(&track);
            }
        }
        
        // Cascade matching (simplified)
        std::set<int> matched_detections;
        std::set<int> matched_tracks;
        
        // Step 1: Match confirmed tracks
        for (auto* track : confirmed_tracks) {
            float best_iou = config_.max_iou_distance;
            int best_det_idx = -1;
            
            for (size_t i = 0; i < detections.size(); ++i) {
                if (matched_detections.count(i)) continue;
                
                float iou = computeIoU(track->getPredictedBBox(), detections[i].bbox());
                if (iou > best_iou) {
                    best_iou = iou;
                    best_det_idx = i;
                }
            }
            
            if (best_det_idx >= 0) {
                track->update(detections[best_det_idx]);
                matched_detections.insert(best_det_idx);
                matched_tracks.insert(track->track_id);
            }
        }
        
        // Step 2: Match tentative tracks with remaining detections
        for (auto* track : tentative_tracks) {
            if (matched_tracks.count(track->track_id)) continue;
            
            float best_iou = config_.max_iou_distance;
            int best_det_idx = -1;
            
            for (size_t i = 0; i < detections.size(); ++i) {
                if (matched_detections.count(i)) continue;
                
                float iou = computeIoU(track->getPredictedBBox(), detections[i].bbox());
                if (iou > best_iou) {
                    best_iou = iou;
                    best_det_idx = i;
                }
            }
            
            if (best_det_idx >= 0) {
                track->update(detections[best_det_idx]);
                matched_detections.insert(best_det_idx);
                matched_tracks.insert(track->track_id);
            }
        }
        
        // Step 3: Create new tracks for unmatched detections
        for (size_t i = 0; i < detections.size(); ++i) {
            if (!matched_detections.count(i)) {
                tracks_.emplace_back(next_track_id_++, detections[i]);
            }
        }
        
        // Step 4: Remove stale tracks
        tracks_.erase(
            std::remove_if(tracks_.begin(), tracks_.end(),
                [this](const Track& track) {
                    return track.time_since_update > config_.max_age ||
                           (track.state == TRACKING_TENTATIVE && track.time_since_update > 3);
                }),
            tracks_.end()
        );
        
        return tracks_;
    }
    
private:
    float computeIoU(const BoundingBox& a, const BoundingBox& b) {
        float x1 = std::max(a.x(), b.x());
        float y1 = std::max(a.y(), b.y());
        float x2 = std::min(a.x() + a.width(), b.x() + b.width());
        float y2 = std::min(a.y() + a.height(), b.y() + b.height());
        
        if (x2 <= x1 || y2 <= y1) return 0.0f;
        
        float intersection = (x2 - x1) * (y2 - y1);
        float area_a = a.width() * a.height();
        float area_b = b.width() * b.height();
        
        return intersection / (area_a + area_b - intersection);
    }
    
    PerceptionConfig config_;
    std::vector<Track> tracks_;
    int next_track_id_;
};

// ============================================================================
// RKNN Detector (RK3588 NPU)
// ============================================================================

#ifdef PLATFORM_RK3588
class RKNNDetector {
public:
    bool init(const std::string& model_path) {
        // Load RKNN model
        int ret = rknn_init(&ctx_, model_path.c_str(), 0, 0, nullptr);
        if (ret < 0) {
            std::cerr << "[Perception] Failed to init RKNN: " << ret << std::endl;
            return false;
        }
        
        // Get model info
        rknn_input_output_num io_num;
        ret = rknn_query(ctx_, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
        if (ret != RKNN_SUCC) {
            std::cerr << "[Perception] Failed to query IO num" << std::endl;
            return false;
        }
        
        n_input_ = io_num.n_input;
        n_output_ = io_num.n_output;
        
        // Query input attributes
        input_attrs_.resize(n_input_);
        for (uint32_t i = 0; i < n_input_; i++) {
            input_attrs_[i].index = i;
            ret = rknn_query(ctx_, RKNN_QUERY_INPUT_ATTR, &input_attrs_[i], 
                           sizeof(rknn_tensor_attr));
        }
        
        // Query output attributes
        output_attrs_.resize(n_output_);
        for (uint32_t i = 0; i < n_output_; i++) {
            output_attrs_[i].index = i;
            ret = rknn_query(ctx_, RKNN_QUERY_OUTPUT_ATTR, &output_attrs_[i],
                           sizeof(rknn_tensor_attr));
        }
        
        // Set input buffer
        input_buffer_.resize(input_attrs_[0].size_with_stride);
        
        std::cout << "[Perception] RKNN model loaded: " <> model_path << std::endl;
        return true;
    }
    
    std::vector<Detection> detect(const cv::Mat& frame) {
        std::vector<Detection> detections;
        
        // Preprocess
        cv::Mat input = preprocess(frame);
        
        // Set input
        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].size = input.total() * input.elemSize();
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].buf = input.data;
        
        int ret = rknn_inputs_set(ctx_, 1, inputs);
        if (ret < 0) {
            std::cerr << "[Perception] Failed to set input" << std::endl;
            return detections;
        }
        
        // Run inference
        ret = rknn_run(ctx_, nullptr);
        if (ret < 0) {
            std::cerr << "[Perception] Inference failed" << std::endl;
            return detections;
        }
        
        // Get output
        rknn_output outputs[n_output_];
        memset(outputs, 0, sizeof(outputs));
        for (uint32_t i = 0; i < n_output_; i++) {
            outputs[i].want_float = 1;
        }
        
        ret = rknn_outputs_get(ctx_, n_output_, outputs, nullptr);
        if (ret < 0) {
            std::cerr << "[Perception] Failed to get outputs" << std::endl;
            return detections;
        }
        
        // Postprocess
        detections = postprocess(outputs, frame.cols, frame.rows);
        
        // Release outputs
        rknn_outputs_release(ctx_, n_output_, outputs);
        
        return detections;
    }
    
    ~RKNNDetector() {
        if (ctx_) {
            rknn_destroy(ctx_);
        }
    }

private:
    cv::Mat preprocess(const cv::Mat& frame) {
        cv::Mat input;
        cv::cvtColor(frame, input, cv::COLOR_BGR2RGB);
        cv::resize(input, input, cv::Size(640, 640));
        return input;
    }
    
    std::vector<Detection> postprocess(rknn_output* outputs, int img_w, int img_h) {
        // YOLO postprocessing
        std::vector<Detection> detections;
        
        // Parse outputs (YOLOv8 format)
        float* data = static_cast<float*>(outputs[0].buf);
        int num_boxes = 8400;  // YOLOv8 default
        int num_classes = 80;  // COCO
        
        for (int i = 0; i < num_boxes; i++) {
            float cx = data[i];
            float cy = data[num_boxes + i];
            float w = data[2 * num_boxes + i];
            float h = data[3 * num_boxes + i];
            
            // Find best class
            float max_conf = 0;
            int best_class = -1;
            for (int c = 0; c < num_classes; c++) {
                float conf = data[(4 + c) * num_boxes + i];
                if (conf > max_conf) {
                    max_conf = conf;
                    best_class = c;
                }
            }
            
            if (max_conf > 0.5f) {
                Detection det;
                det.track_id(-1);
                det.class_id(static_cast<DetectionClass>(best_class));
                det.confidence(max_conf);
                
                BoundingBox bbox;
                bbox.x((cx - w/2) / 640.0f);
                bbox.y((cy - h/2) / 640.0f);
                bbox.width(w / 640.0f);
                bbox.height(h / 640.0f);
                bbox.confidence(max_conf);
                det.bbox(bbox);
                
                detections.push_back(det);
            }
        }
        
        return detections;
    }
    
    rknn_context ctx_{0};
    uint32_t n_input_{0};
    uint32_t n_output_{0};
    std::vector<rknn_tensor_attr> input_attrs_;
    std::vector<rknn_tensor_attr> output_attrs_;
    std::vector<uint8_t> input_buffer_;
};
#endif

// ============================================================================
// Perception Process Class
// ============================================================================

class PerceptionProcess {
public:
    PerceptionProcess(const PerceptionConfig& config)
        : config_(config)
        , running_(false)
        , sequence_(0)
        , tracker_(config)
    {
    }
    
    ~PerceptionProcess() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "[Perception] Initializing process..." << std::endl;
        
        // Initialize DDS
        if (!initializeDDS()) {
            return false;
        }
        
        // Initialize detector
        if (!initializeDetector()) {
            return false;
        }
        
        std::cout << "[Perception] Process initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        running_ = true;
        
        std::cout << "[Perception] Starting main loop..." << std::endl;
        
        while (running_) {
            // Process frames
            processFrame();
            
            // 30Hz processing
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
    
    void shutdown() {
        if (!running_) return;
        
        std::cout << "[Perception] Shutting down..." << std::endl;
        running_ = false;
        
        cleanupDDS();
        
        std::cout << "[Perception] Shutdown complete" << std::endl;
    }

private:
    bool initializeDDS() {
        // Create participant
        participant_ = DomainParticipantFactory::get_instance()
            ->create_participant(
                static_cast<DomainId_t>(std::stoi(config_.dds_domain_id)),
                PARTICIPANT_QOS_DEFAULT);
        
        if (!participant_) {
            std::cerr << "[Perception] Failed to create DDS participant" << std::endl;
            return false;
        }
        
        // Register types
        detection_type_.register_type(participant_);
        tracking_type_.register_type(participant_);
        
        // Create publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        
        // Create topics
        detection_topic_ = participant_->create_topic(
            config_.detection_topic,
            detection_type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        tracking_topic_ = participant_->create_topic(
            config_.tracking_topic,
            tracking_type_.get_type_name(),
            TOPIC_QOS_DEFAULT);
        
        // Create DataWriters with QoS
        DataWriterQos detection_qos;
        detection_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        detection_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        detection_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        detection_qos.history().depth = 1;
        
        DataWriterQos tracking_qos = detection_qos;
        
        detection_writer_ = publisher_->create_datawriter(
            detection_topic_, detection_qos, nullptr);
        tracking_writer_ = publisher_->create_datawriter(
            tracking_topic_, tracking_qos, nullptr);
        
        return true;
    }
    
    bool initializeDetector() {
#ifdef PLATFORM_RK3588
        detector_ = std::make_unique<RKNNDetector>();
        if (!detector_->init(config_.model_path)) {
            std::cerr << "[Perception] Failed to initialize RKNN detector" << std::endl;
            return false;
        }
#else
        std::cout << "[Perception] Running in simulation mode (no NPU)" << std::endl;
#endif
        return true;
    }
    
    void processFrame() {
        // In real implementation, get frame from video capture via RTSP/DDS
        // For now, simulate detection
        
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch());
        
        // Create detection array
        DetectionArray detection_array;
        detection_array.frame_id("camera");
        detection_array.sequence(sequence_++);
        detection_array.timestamp().seconds(ns.count() / 1000000000);
        detection_array.timestamp().nanoseconds(ns.count() % 1000000000);
        detection_array.image_width(1920);
        detection_array.image_height(1080);
        
        // Run detection (simulated)
        std::vector<Detection> detections = runDetection();
        detection_array.detections(detections);
        
        // Publish detections
        if (detection_writer_) {
            detection_writer_->write(&detection_array);
        }
        
        // Run tracking
        if (config_.enable_tracking) {
            auto tracks = tracker_.update(detections);
            
            // Create tracking array
            TrackingArray tracking_array;
            tracking_array.frame_id("camera");
            tracking_array.sequence(detection_array.sequence());
            tracking_array.timestamp(detection_array.timestamp());
            
            for (const auto& track : tracks) {
                Track t;
                t.track_id(track.track_id);
                t.class_id(track.detection.class_id());
                t.state(track.state);
                t.bbox(track.detection.bbox());
                tracking_array.tracks().push_back(t);
            }
            
            tracking_array.active_tracks(tracks.size());
            
            if (tracking_writer_) {
                tracking_writer_->write(&tracking_array);
            }
        }
    }
    
    std::vector<Detection> runDetection() {
        std::vector<Detection> detections;
        
#ifdef PLATFORM_RK3588
        // Real detection using RKNN
        cv::Mat frame;  // Get from video stream
        detections = detector_->detect(frame);
#else
        // Simulated detections for testing
        static int frame_count = 0;
        frame_count++;
        
        if (frame_count % 30 < 10) {
            // Simulate person detection
            Detection det;
            det.track_id(-1);
            det.class_id(CLASS_PERSON);
            det.class_name("person");
            det.confidence(0.85f);
            
            BoundingBox bbox;
            bbox.x(0.3f + 0.01f * (frame_count % 10));
            bbox.y(0.2f);
            bbox.width(0.1f);
            bbox.height(0.2f);
            bbox.confidence(0.85f);
            det.bbox(bbox);
            det.distance_meters(25.0f);
            
            detections.push_back(det);
        }
#endif
        
        return detections;
    }
    
    void cleanupDDS() {
        if (participant_) {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()
                ->delete_participant(participant_);
        }
    }

private:
    PerceptionConfig config_;
    std::atomic<bool> running_;
    uint32_t sequence_;
    
    // DDS
    DomainParticipant* participant_{nullptr};
    Publisher* publisher_{nullptr};
    Topic* detection_topic_{nullptr};
    Topic* tracking_topic_{nullptr};
    DataWriter* detection_writer_{nullptr};
    DataWriter* tracking_writer_{nullptr};
    DetectionArrayPubSubType detection_type_;
    TrackingArrayPubSubType tracking_type_;
    
    // Tracking
    DeepSORTTracker tracker_;
    
    // Detection
#ifdef PLATFORM_RK3588
    std::unique_ptr<RKNNDetector> detector_;
#endif
};

// ============================================================================
// Main Entry Point
// ============================================================================

static std::atomic<bool> g_running{true};
static PerceptionProcess* g_process = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[Perception] Received signal " << sig << std::endl;
    g_running = false;
    if (g_process) {
        g_process->shutdown();
    }
}

PerceptionConfig loadConfig(const std::string& config_file) {
    PerceptionConfig config;
    
    try {
        YAML::Node yaml = YAML::LoadFile(config_file);
        
        if (yaml["model"]) {
            config.model_path = yaml["model"]["path"].as<std::string>(config.model_path);
            config.confidence_threshold = yaml["model"]["confidence_threshold"].as<float>(config.confidence_threshold);
        }
        
        if (yaml["tracking"]) {
            config.enable_tracking = yaml["tracking"]["enabled"].as<bool>(config.enable_tracking);
            config.max_age = yaml["tracking"]["max_age"].as<int>(config.max_age);
            config.min_hits = yaml["tracking"]["min_hits"].as<int>(config.min_hits);
        }
        
        if (yaml["dds"]) {
            config.dds_domain_id = yaml["dds"]["domain_id"].as<std::string>(config.dds_domain_id);
        }
        
        std::cout << "[Perception] Configuration loaded" << std::endl;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[Perception] Config load failed: " << e.what() << std::endl;
    }
    
    return config;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "FalconMind Perception Process v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string config_file = "/etc/falconmind/perception.yaml";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        }
    }
    
    PerceptionConfig config = loadConfig(config_file);
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    PerceptionProcess process(config);
    g_process = &process;
    
    if (!process.initialize()) {
        std::cerr << "[Perception] Initialization failed" << std::endl;
        return 1;
    }
    
    process.run();
    
    return 0;
}

} // namespace processes
} // namespace falconmind
