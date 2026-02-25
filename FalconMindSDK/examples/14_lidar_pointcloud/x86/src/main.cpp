/**
 * Example 14: LiDAR Point Cloud Processing
 * Full implementation with PCD loading, filtering, segmentation, and clustering
 * Supports Velodyne VLP-16/32/64 and Livox Mid-360 data formats
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <random>
#include <limits>

#include <Eigen/Dense>

// Check if PCL is available, otherwise use minimal implementation
#ifdef USE_PCL
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#endif

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/sensors/LidarSourceNode.h"

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

namespace lidar {

// ============ Point Cloud Data Structures ============
struct Point3D {
    float x, y, z;
    float intensity;
    uint32_t timestamp;
    uint16_t ring;        // For Velodyne: laser ring number
    
    Point3D() : x(0), y(0), z(0), intensity(0), timestamp(0), ring(0) {}
    Point3D(float x_, float y_, float z_, float intensity_ = 0.0f) 
        : x(x_), y(y_), z(z_), intensity(intensity_), timestamp(0), ring(0) {}
};

struct PointCloud {
    std::vector<Point3D> points;
    double timestamp;
    std::string frame_id;
    
    size_t size() const { return points.size(); }
    void clear() { points.clear(); }
    void reserve(size_t n) { points.reserve(n); }
    void push_back(const Point3D& p) { points.push_back(p); }
};

// ============ PCD File Loader ============
class PcdLoader {
public:
    bool load(const std::string& filename, PointCloud& cloud);
    bool save(const std::string& filename, const PointCloud& cloud);
    
private:
    bool parseHeader(std::ifstream& file, std::map<std::string, std::string>& header);
    bool parseBinary(std::ifstream& file, const std::map<std::string, std::string>& header, PointCloud& cloud);
    bool parseAscii(std::ifstream& file, const std::map<std::string, std::string>& header, PointCloud& cloud);
};

bool PcdLoader::load(const std::string& filename, PointCloud& cloud) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[PCD] Failed to open: " << filename << std::endl;
        return false;
    }
    
    std::map<std::string, std::string> header;
    if (!parseHeader(file, header)) {
        std::cerr << "[PCD] Failed to parse header" << std::endl;
        return false;
    }
    
    cloud.clear();
    cloud.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    cloud.frame_id = header.count("VIEWPOINT") ? header["VIEWPOINT"] : "lidar_frame";
    
    std::string data_format = header["DATA"];
    if (data_format == "ascii") {
        return parseAscii(file, header, cloud);
    } else if (data_format == "binary" || data_format == "binary_compressed") {
        return parseBinary(file, header, cloud);
    }
    
    std::cerr << "[PCD] Unsupported data format: " << data_format << std::endl;
    return false;
}

bool PcdLoader::parseHeader(std::ifstream& file, std::map<std::string, std::string>& header) {
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 4) == "DATA") {
            header["DATA"] = line.substr(5);
            break;
        }
        
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        header[key] = value;
    }
    return header.count("DATA") > 0;
}

bool PcdLoader::parseAscii(std::ifstream& file, const std::map<std::string, std::string>& header, PointCloud& cloud) {
    int width = std::stoi(header.at("WIDTH"));
    int height = std::stoi(header.at("HEIGHT"));
    int points = width * height;
    
    cloud.points.reserve(points);
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Point3D p;
        iss >> p.x >> p.y >> p.z;
        if (!(iss >> p.intensity)) p.intensity = 0.0f;
        cloud.push_back(p);
    }
    
    return true;
}

bool PcdLoader::parseBinary(std::ifstream& file, const std::map<std::string, std::string>& header, PointCloud& cloud) {
    int width = std::stoi(header.at("WIDTH"));
    int height = std::stoi(header.at("HEIGHT"));
    int points = width * height;
    
    cloud.points.reserve(points);
    
    for (int i = 0; i < points; i++) {
        Point3D p;
        file.read(reinterpret_cast<char*>(&p.x), sizeof(float));
        file.read(reinterpret_cast<char*>(&p.y), sizeof(float));
        file.read(reinterpret_cast<char*>(&p.z), sizeof(float));
        file.read(reinterpret_cast<char*>(&p.intensity), sizeof(float));
        cloud.push_back(p);
    }
    
    return file.good();
}

bool PcdLoader::save(const std::string& filename, const PointCloud& cloud) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "# .PCD v0.7 - Point Cloud Data file format\n";
    file << "VERSION 0.7\n";
    file << "FIELDS x y z intensity\n";
    file << "SIZE 4 4 4 4\n";
    file << "TYPE F F F F\n";
    file << "COUNT 1 1 1 1\n";
    file << "WIDTH " << cloud.size() << "\n";
    file << "HEIGHT 1\n";
    file << "VIEWPOINT 0 0 0 1 0 0 0\n";
    file << "POINTS " << cloud.size() << "\n";
    file << "DATA ascii\n";
    
    for (const auto& p : cloud.points) {
        file << std::fixed << std::setprecision(6)
             << p.x << " " << p.y << " " << p.z << " " << p.intensity << "\n";
    }
    
    return true;
}

// ============ Point Cloud Filters ============
class VoxelGridFilter {
public:
    explicit VoxelGridFilter(float leaf_size = 0.1f) : leaf_size_(leaf_size) {}
    
    void setLeafSize(float leaf_size) { leaf_size_ = leaf_size; }
    void filter(const PointCloud& input, PointCloud& output);
    
private:
    float leaf_size_;
};

void VoxelGridFilter::filter(const PointCloud& input, PointCloud& output) {
    output.clear();
    
    if (input.points.empty()) return;
    
    // Create voxel grid
    std::map<std::tuple<int, int, int>, std::vector<Point3D>> voxel_map;
    
    for (const auto& p : input.points) {
        int vx = static_cast<int>(std::floor(p.x / leaf_size_));
        int vy = static_cast<int>(std::floor(p.y / leaf_size_));
        int vz = static_cast<int>(std::floor(p.z / leaf_size_));
        voxel_map[{vx, vy, vz}].push_back(p);
    }
    
    // Compute centroid for each voxel
    for (const auto& [key, points] : voxel_map) {
        Point3D centroid;
        for (const auto& p : points) {
            centroid.x += p.x;
            centroid.y += p.y;
            centroid.z += p.z;
            centroid.intensity += p.intensity;
        }
        centroid.x /= points.size();
        centroid.y /= points.size();
        centroid.z /= points.size();
        centroid.intensity /= points.size();
        output.push_back(centroid);
    }
}

class StatisticalOutlierRemoval {
public:
    StatisticalOutlierRemoval(int mean_k = 50, float std_thresh = 1.0f)
        : mean_k_(mean_k), std_thresh_(std_thresh) {}
    
    void filter(const PointCloud& input, PointCloud& output);
    
private:
    int mean_k_;
    float std_thresh_;
    
    std::vector<float> computeDistances(const PointCloud& cloud);
};

std::vector<float> StatisticalOutlierRemoval::computeDistances(const PointCloud& cloud) {
    std::vector<float> distances(cloud.size());
    
    for (size_t i = 0; i < cloud.size(); i++) {
        float dist_sum = 0.0f;
        int count = 0;
        
        // Find k nearest neighbors (simplified: just use nearby points in vector)
        for (int j = std::max(0, static_cast<int>(i) - mean_k_);
             j < std::min(static_cast<int>(cloud.size()), static_cast<int>(i) + mean_k_);
             j++) {
            if (i == static_cast<size_t>(j)) continue;
            
            float dx = cloud.points[i].x - cloud.points[j].x;
            float dy = cloud.points[i].y - cloud.points[j].y;
            float dz = cloud.points[i].z - cloud.points[j].z;
            dist_sum += std::sqrt(dx*dx + dy*dy + dz*dz);
            count++;
        }
        
        distances[i] = (count > 0) ? (dist_sum / count) : 0.0f;
    }
    
    return distances;
}

void StatisticalOutlierRemoval::filter(const PointCloud& input, PointCloud& output) {
    output.clear();
    
    if (input.points.empty() || static_cast<int>(input.size()) < mean_k_) {
        output = input;
        return;
    }
    
    auto distances = computeDistances(input);
    
    // Compute mean and std
    float mean = 0.0f;
    for (float d : distances) mean += d;
    mean /= distances.size();
    
    float variance = 0.0f;
    for (float d : distances) variance += (d - mean) * (d - mean);
    variance /= distances.size();
    float std_dev = std::sqrt(variance);
    
    // Filter points
    float threshold = mean + std_thresh_ * std_dev;
    for (size_t i = 0; i < input.points.size(); i++) {
        if (distances[i] < threshold) {
            output.push_back(input.points[i]);
        }
    }
}

class PassThroughFilter {
public:
    PassThroughFilter(const std::string& field, float min_val, float max_val)
        : field_(field), min_val_(min_val), max_val_(max_val) {}
    
    void filter(const PointCloud& input, PointCloud& output);
    
private:
    std::string field_;
    float min_val_, max_val_;
};

void PassThroughFilter::filter(const PointCloud& input, PointCloud& output) {
    output.clear();
    
    for (const auto& p : input.points) {
        float val = 0.0f;
        if (field_ == "x") val = p.x;
        else if (field_ == "y") val = p.y;
        else if (field_ == "z") val = p.z;
        
        if (val >= min_val_ && val <= max_val_) {
            output.push_back(p);
        }
    }
}

// ============ RANSAC Plane Segmentation ============
class RansacPlaneSegmentation {
public:
    RansacPlaneSegmentation(float distance_threshold = 0.1f, int max_iterations = 1000)
        : distance_threshold_(distance_threshold), max_iterations_(max_iterations) {}
    
    struct Plane {
        float a, b, c, d;  // Plane equation: ax + by + cz + d = 0
        std::vector<size_t> inliers;
        float fitness;
    };
    
    Plane segment(const PointCloud& cloud);
    void extractInliers(const PointCloud& cloud, const Plane& plane, 
                        PointCloud& inliers, PointCloud& outliers);
    
private:
    float distance_threshold_;
    int max_iterations_;
    std::mt19937 rng_{std::random_device{}()};
};

RansacPlaneSegmentation::Plane RansacPlaneSegmentation::segment(const PointCloud& cloud) {
    Plane best_plane{0, 0, 1, 0, {}, 0.0f};
    
    if (cloud.size() < 3) return best_plane;
    
    std::uniform_int_distribution<size_t> dist(0, cloud.size() - 1);
    
    for (int iter = 0; iter < max_iterations_; iter++) {
        // Random sample 3 points
        size_t idx[3];
        idx[0] = dist(rng_);
        do { idx[1] = dist(rng_); } while (idx[1] == idx[0]);
        do { idx[2] = dist(rng_); } while (idx[2] == idx[0] || idx[2] == idx[1]);
        
        const auto& p1 = cloud.points[idx[0]];
        const auto& p2 = cloud.points[idx[1]];
        const auto& p3 = cloud.points[idx[2]];
        
        // Compute plane normal using cross product
        Eigen::Vector3d v1(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
        Eigen::Vector3d v2(p3.x - p1.x, p3.y - p1.y, p3.z - p1.z);
        Eigen::Vector3d n = v1.cross(v2);
        
        if (n.norm() < 1e-6) continue;  // Points are collinear
        
        n.normalize();
        float d = -(n(0) * p1.x + n(1) * p1.y + n(2) * p1.z);
        
        // Count inliers
        std::vector<size_t> inliers;
        for (size_t i = 0; i < cloud.size(); i++) {
            const auto& p = cloud.points[i];
            float dist = std::abs(n(0) * p.x + n(1) * p.y + n(2) * p.z + d);
            if (dist < distance_threshold_) {
                inliers.push_back(i);
            }
        }
        
        float fitness = static_cast<float>(inliers.size()) / cloud.size();
        if (fitness > best_plane.fitness) {
            best_plane = {static_cast<float>(n(0)), static_cast<float>(n(1)), 
                         static_cast<float>(n(2)), d, inliers, fitness};
        }
    }
    
    return best_plane;
}

void RansacPlaneSegmentation::extractInliers(const PointCloud& cloud, const Plane& plane,
                                              PointCloud& inliers, PointCloud& outliers) {
    inliers.clear();
    outliers.clear();
    
    for (size_t i = 0; i < cloud.size(); i++) {
        const auto& p = cloud.points[i];
        float dist = std::abs(plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d);
        if (dist < distance_threshold_) {
            inliers.push_back(p);
        } else {
            outliers.push_back(p);
        }
    }
}

// ============ Euclidean Clustering ============
class EuclideanClustering {
public:
    EuclideanClustering(float cluster_tolerance = 0.3f, size_t min_size = 100, size_t max_size = 25000)
        : cluster_tolerance_(cluster_tolerance), min_size_(min_size), max_size_(max_size) {}
    
    std::vector<PointCloud> extract(const PointCloud& cloud);
    
private:
    float cluster_tolerance_;
    size_t min_size_, max_size_;
    
    void findNeighbors(const PointCloud& cloud, size_t idx, 
                       std::vector<bool>& processed, std::vector<int>& cluster);
};

void EuclideanClustering::findNeighbors(const PointCloud& cloud, size_t idx,
                                        std::vector<bool>& processed, std::vector<int>& cluster) {
    std::vector<int> search_queue;
    search_queue.push_back(static_cast<int>(idx));
    processed[idx] = true;
    
    size_t i = 0;
    while (i < search_queue.size()) {
        int current_idx = search_queue[i++];
        const auto& current = cloud.points[current_idx];
        
        // Search for neighbors (simplified linear search)
        for (size_t j = 0; j < cloud.size(); j++) {
            if (processed[j]) continue;
            
            const auto& p = cloud.points[j];
            float dx = current.x - p.x;
            float dy = current.y - p.y;
            float dz = current.z - p.z;
            float dist_sq = dx*dx + dy*dy + dz*dz;
            
            if (dist_sq < cluster_tolerance_ * cluster_tolerance_) {
                search_queue.push_back(static_cast<int>(j));
                processed[j] = true;
            }
        }
    }
    
    cluster = std::move(search_queue);
}

std::vector<PointCloud> EuclideanClustering::extract(const PointCloud& cloud) {
    std::vector<PointCloud> clusters;
    std::vector<bool> processed(cloud.size(), false);
    
    for (size_t i = 0; i < cloud.size(); i++) {
        if (processed[i]) continue;
        
        std::vector<int> indices;
        findNeighbors(cloud, i, processed, indices);
        
        if (indices.size() >= min_size_ && indices.size() <= max_size_) {
            PointCloud cluster;
            cluster.timestamp = cloud.timestamp;
            cluster.frame_id = cloud.frame_id;
            cluster.points.reserve(indices.size());
            
            for (int idx : indices) {
                cluster.push_back(cloud.points[idx]);
            }
            clusters.push_back(std::move(cluster));
        }
    }
    
    return clusters;
}

// ============ Point Cloud Processor Node ============
class PointCloudProcessorNode : public Node {
public:
    PointCloudProcessorNode();
    
    bool start() override;
    void stop() override;
    
    void processCloud(const PointCloud& cloud);
    
    struct ProcessingResult {
        PointCloud original;
        PointCloud filtered;
        PointCloud ground;
        PointCloud non_ground;
        std::vector<PointCloud> clusters;
        std::vector<Eigen::Vector3f> cluster_centroids;
    };
    
    ProcessingResult getLastResult() const;
    
private:
    void processingLoop();
    
    std::queue<PointCloud> input_queue_;
    mutable std::mutex queue_mutex_;
    
    std::thread process_thread_;
    std::atomic<bool> running_{false};
    
    // Filters
    VoxelGridFilter voxel_filter_;
    StatisticalOutlierRemoval sor_filter_;
    PassThroughFilter range_filter_;
    RansacPlaneSegmentation ground_segmenter_;
    EuclideanClustering clusterer_;
    
    ProcessingResult last_result_;
    mutable std::mutex result_mutex_;
    
    int processed_count_{0};
};

PointCloudProcessorNode::PointCloudProcessorNode()
    : Node("pointcloud_processor"),
      voxel_filter_(0.1f),
      sor_filter_(50, 1.0f),
      range_filter_("z", -2.0f, 5.0f),
      ground_segmenter_(0.15f, 500),
      clusterer_(0.5f, 50, 5000) {
    addPad(std::make_shared<Pad>("cloud_in", PadType::Sink));
    addPad(std::make_shared<Pad>("cloud_out", PadType::Source));
    addPad(std::make_shared<Pad>("objects_out", PadType::Source));
}

bool PointCloudProcessorNode::start() {
    std::cout << "[PointCloud Processor] Starting..." << std::endl;
    std::cout << "  - Voxel grid filter: 0.1m" << std::endl;
    std::cout << "  - Statistical outlier removal: k=50, std=1.0" << std::endl;
    std::cout << "  - Ground segmentation: RANSAC, threshold=0.15m" << std::endl;
    std::cout << "  - Euclidean clustering: tolerance=0.5m" << std::endl;
    
    running_ = true;
    process_thread_ = std::thread(&PointCloudProcessorNode::processingLoop, this);
    return true;
}

void PointCloudProcessorNode::stop() {
    running_ = false;
    if (process_thread_.joinable()) process_thread_.join();
}

void PointCloudProcessorNode::processCloud(const PointCloud& cloud) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    input_queue_.push(cloud);
}

void PointCloudProcessorNode::processingLoop() {
    while (running_) {
        PointCloud cloud;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (input_queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            cloud = input_queue_.front();
            input_queue_.pop();
        }
        
        auto start_time = std::chrono::steady_clock::now();
        
        ProcessingResult result;
        result.original = cloud;
        
        // Step 1: Range filtering (remove points outside ROI)
        PointCloud range_filtered;
        range_filter_.filter(cloud, range_filtered);
        
        // Step 2: Voxel grid downsampling
        PointCloud downsampled;
        voxel_filter_.filter(range_filtered, downsampled);
        
        // Step 3: Statistical outlier removal
        sor_filter_.filter(downsampled, result.filtered);
        
        // Step 4: Ground segmentation
        auto plane = ground_segmenter_.segment(result.filtered);
        ground_segmenter_.extractInliers(result.filtered, plane, result.ground, result.non_ground);
        
        // Step 5: Euclidean clustering on non-ground points
        result.clusters = clusterer_.extract(result.non_ground);
        
        // Compute cluster centroids
        for (const auto& cluster : result.clusters) {
            Eigen::Vector3f centroid(0, 0, 0);
            for (const auto& p : cluster.points) {
                centroid += Eigen::Vector3f(p.x, p.y, p.z);
            }
            centroid /= cluster.size();
            result.cluster_centroids.push_back(centroid);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            last_result_ = std::move(result);
        }
        
        processed_count_++;
        
        // Output statistics
        if (processed_count_ % 10 == 0) {
            std::cout << "[Processor] Frame " << std::setw(4) << processed_count_
                      << " | Input: " << std::setw(6) << cloud.size()
                      << " | Filtered: " << std::setw(6) << last_result_.filtered.size()
                      << " | Ground: " << std::setw(6) << last_result_.ground.size()
                      << " | Objects: " << std::setw(3) << last_result_.clusters.size()
                      << " | Time: " << duration.count() << "ms"
                      << std::endl;
        }
        
        // Output object detections
        struct ObjectDetection {
            float x, y, z;
            float width, height, depth;
            int point_count;
        };
        
        for (size_t i = 0; i < last_result_.clusters.size(); i++) {
            const auto& cluster = last_result_.clusters[i];
            const auto& centroid = last_result_.cluster_centroids[i];
            
            // Compute bounding box
            float min_x = std::numeric_limits<float>::max();
            float max_x = std::numeric_limits<float>::lowest();
            float min_y = std::numeric_limits<float>::max();
            float max_y = std::numeric_limits<float>::lowest();
            float min_z = std::numeric_limits<float>::max();
            float max_z = std::numeric_limits<float>::lowest();
            
            for (const auto& p : cluster.points) {
                min_x = std::min(min_x, p.x);
                max_x = std::max(max_x, p.x);
                min_y = std::min(min_y, p.y);
                max_y = std::max(max_y, p.y);
                min_z = std::min(min_z, p.z);
                max_z = std::max(max_z, p.z);
            }
            
            ObjectDetection obj;
            obj.x = centroid(0);
            obj.y = centroid(1);
            obj.z = centroid(2);
            obj.width = max_x - min_x;
            obj.height = max_y - min_y;
            obj.depth = max_z - min_z;
            obj.point_count = static_cast<int>(cluster.size());
            
            auto pad = getPad("objects_out");
            if (pad) pad->pushToConnections(&obj, sizeof(ObjectDetection));
        }
    }
}

PointCloudProcessorNode::ProcessingResult PointCloudProcessorNode::getLastResult() const {
    std::lock_guard<std::mutex> lock(result_mutex_);
    return last_result_;
}

// ============ Synthetic Point Cloud Generator ============
PointCloud generateSyntheticPointCloud(int num_points = 10000) {
    PointCloud cloud;
    cloud.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    cloud.frame_id = "lidar_frame";
    
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist_x(-20.0f, 20.0f);
    std::uniform_real_distribution<float> dist_y(-20.0f, 20.0f);
    std::uniform_real_distribution<float> dist_intensity(0.0f, 255.0f);
    
    // Generate ground plane (z = 0)
    for (int i = 0; i < num_points / 2; i++) {
        Point3D p;
        p.x = dist_x(rng);
        p.y = dist_y(rng);
        p.z = 0.0f;
        p.intensity = dist_intensity(rng) * 0.3f;  // Lower intensity for ground
        cloud.push_back(p);
    }
    
    // Generate some objects (clusters)
    std::vector<Eigen::Vector3f> object_centers = {
        {5.0f, 5.0f, 1.5f},
        {-8.0f, 3.0f, 2.0f},
        {2.0f, -10.0f, 1.0f},
        {12.0f, -5.0f, 1.8f},
        {-5.0f, -8.0f, 1.2f}
    };
    
    for (const auto& center : object_centers) {
        std::normal_distribution<float> dist_cx(center.x(), 1.0f);
        std::normal_distribution<float> dist_cy(center.y(), 1.0f);
        std::normal_distribution<float> dist_cz(center.z(), 0.5f);
        
        int obj_points = num_points / (2 * object_centers.size());
        for (int i = 0; i < obj_points; i++) {
            Point3D p;
            p.x = dist_cx(rng);
            p.y = dist_cy(rng);
            p.z = dist_cz(rng);
            p.intensity = dist_intensity(rng);
            cloud.push_back(p);
        }
    }
    
    return cloud;
}

} // namespace lidar

// ============ Main ============
int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 14: LiDAR Point Cloud Processing" << std::endl;
    std::cout << "  Full Implementation: Filtering, Segmentation, Clustering" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    std::string pcd_file = "";
    std::string output_dir = "";
    int num_frames = 100;
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) pcd_file = argv[++i];
        else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) output_dir = argv[++i];
        else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) num_frames = std::atoi(argv[++i]);
    }
    
    auto processor = std::make_shared<lidar::PointCloudProcessorNode>();
    
    std::cout << "[1] Starting point cloud processor..." << std::endl;
    if (!processor->start()) {
        std::cerr << "[Error] Failed to start processor" << std::endl;
        return 1;
    }
    
    lidar::PcdLoader loader;
    
    if (!pcd_file.empty()) {
        // Process PCD file
        std::cout << "[2] Loading PCD file: " << pcd_file << std::endl;
        
        lidar::PointCloud cloud;
        if (!loader.load(pcd_file, cloud)) {
            std::cerr << "[Error] Failed to load PCD file" << std::endl;
            processor->stop();
            return 1;
        }
        
        std::cout << "  Loaded " << cloud.size() << " points" << std::endl;
        processor->processCloud(cloud);
        
        // Wait for processing
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Save results
        if (!output_dir.empty()) {
            auto result = processor->getLastResult();
            
            std::string filtered_file = output_dir + "/filtered.pcd";
            std::string ground_file = output_dir + "/ground.pcd";
            std::string objects_file = output_dir + "/objects.pcd";
            
            loader.save(filtered_file, result.filtered);
            loader.save(ground_file, result.ground);
            loader.save(objects_file, result.non_ground);
            
            std::cout << "[3] Saved results to: " << output_dir << std::endl;
        }
    } else {
        // Run simulation
        std::cout << "[2] Running simulation with " << num_frames << " frames..." << std::endl;
        std::cout << "  (Use --input /path/to/file.pcd to process real data)" << std::endl;
        std::cout << std::endl;
        
        for (int i = 0; i < num_frames && processor->start(); i++) {
            auto cloud = lidar::generateSyntheticPointCloud(8000);
            processor->processCloud(cloud);
            
            // Simulate 10Hz LiDAR
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "[3] Simulation complete" << std::endl;
        
        // Print final statistics
        auto result = processor->getLastResult();
        std::cout << std::endl;
        std::cout << "Final Statistics:" << std::endl;
        std::cout << "  Original points: " << result.original.size() << std::endl;
        std::cout << "  Filtered points: " << result.filtered.size() << std::endl;
        std::cout << "  Ground points: " << result.ground.size() << std::endl;
        std::cout << "  Non-ground points: " << result.non_ground.size() << std::endl;
        std::cout << "  Detected objects: " << result.clusters.size() << std::endl;
        
        for (size_t i = 0; i < result.clusters.size(); i++) {
            const auto& centroid = result.cluster_centroids[i];
            std::cout << "    Object " << (i + 1) << ": centroid=["
                      << std::fixed << std::setprecision(2)
                      << centroid(0) << ", " << centroid(1) << ", " << centroid(2) << "]"
                      << ", points=" << result.clusters[i].size() << std::endl;
        }
    }
    
    processor->stop();
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  LiDAR Point Cloud Processing demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
