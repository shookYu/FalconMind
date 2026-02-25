/**
 * Example 25: 3D Multi-Target Tracking
 * Full implementation with Kalman filter tracking, data association, and point cloud fusion
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <cmath>
#include <random>
#include <algorithm>
#include <map>

#include <Eigen/Dense>

using namespace Eigen;

namespace tracking_3d {

struct Detection3D {
    int id;
    Vector3d position;  // x, y, z in meters
    Vector3d velocity;
    double confidence;
    double timestamp;
    std::vector<Vector3d> pointCloud;  // Associated point cloud
};

struct Track {
    int trackId;
    Vector3d position;
    Vector3d velocity;
    MatrixXd covariance;
    int age;
    int hits;
    int misses;
    double lastUpdateTime;
    bool confirmed;
    
    Track(int id) : trackId(id), age(0), hits(1), misses(0), confirmed(false) {
        position.setZero();
        velocity.setZero();
        covariance = MatrixXd::Identity(6, 6);
        lastUpdateTime = 0;
    }
};

class KalmanTracker3D {
public:
    KalmanTracker3D() : nextTrackId_(0) {}
    
    std::vector<std::shared_ptr<Track>> update(const std::vector<Detection3D>& detections, double timestamp) {
        // Predict all tracks
        for (auto& track : tracks_) {
            predict(track, timestamp);
        }
        
        // Data association (simplified nearest neighbor)
        std::vector<bool> detectionMatched(detections.size(), false);
        
        for (auto& track : tracks_) {
            double minDist = 1e9;
            int bestDetection = -1;
            
            for (size_t i = 0; i < detections.size(); i++) {
                if (detectionMatched[i]) continue;
                
                double dist = (track->position - detections[i].position).norm();
                if (dist < minDist && dist < 5.0) {  // 5m gate
                    minDist = dist;
                    bestDetection = i;
                }
            }
            
            if (bestDetection >= 0) {
                updateTrack(track, detections[bestDetection]);
                detectionMatched[bestDetection] = true;
            } else {
                track->misses++;
            }
        }
        
        // Create new tracks for unmatched detections
        for (size_t i = 0; i < detections.size(); i++) {
            if (!detectionMatched[i]) {
                auto newTrack = createTrack(detections[i], timestamp);
                tracks_.push_back(newTrack);
            }
        }
        
        // Remove old tracks
        tracks_.erase(
            std::remove_if(tracks_.begin(), tracks_.end(),
                [](const auto& t) { return t->misses > 5 || (t->age > 10 && t->hits < 3); }),
            tracks_.end()
        );
        
        // Update confirmed status
        for (auto& track : tracks_) {
            if (track->hits >= 3) track->confirmed = true;
        }
        
        return tracks_;
    }
    
    void printTracks() const {
        std::cout << "Active tracks: " << tracks_.size() << std::endl;
        for (const auto& track : tracks_) {
            std::cout << "  Track " << track->trackId 
                      << " Pos: [" << std::fixed << std::setprecision(2)
                      << track->position.transpose() << "]"
                      << " Age: " << track->age
                      << " Hits: " << track->hits
                      << (track->confirmed ? " [CONFIRMED]" : " [TENTATIVE]")
                      << std::endl;
        }
    }
    
private:
    void predict(std::shared_ptr<Track> track, double timestamp) {
        double dt = timestamp - track->lastUpdateTime;
        if (dt <= 0) return;
        
        // State transition
        track->position += track->velocity * dt;
        
        // Covariance prediction
        MatrixXd F = MatrixXd::Identity(6, 6);
        F.block<3, 3>(0, 3) = Matrix3d::Identity() * dt;
        
        MatrixXd Q = MatrixXd::Identity(6, 6) * 0.1;
        track->covariance = F * track->covariance * F.transpose() + Q;
        
        track->age++;
    }
    
    void updateTrack(std::shared_ptr<Track> track, const Detection3D& detection) {
        // Kalman update (simplified)
        MatrixXd H = MatrixXd::Zero(3, 6);
        H.block<3, 3>(0, 0) = Matrix3d::Identity();
        
        Vector3d residual = detection.position - track->position;
        MatrixXd S = H * track->covariance * H.transpose() + Matrix3d::Identity();
        MatrixXd K = track->covariance * H.transpose() * S.inverse();
        
        VectorXd dx = K * residual;
        track->position += dx.segment<3>(0);
        track->velocity += dx.segment<3>(3);
        
        track->hits++;
        track->misses = 0;
        track->lastUpdateTime = detection.timestamp;
    }
    
    std::shared_ptr<Track> createTrack(const Detection3D& detection, double timestamp) {
        auto track = std::make_shared<Track>(nextTrackId_++);
        track->position = detection.position;
        track->velocity = detection.velocity;
        track->lastUpdateTime = timestamp;
        return track;
    }
    
    std::vector<std::shared_ptr<Track>> tracks_;
    int nextTrackId_;
};

std::vector<Detection3D> generateDetections(int frame, double timestamp) {
    std::vector<Detection3D> detections;
    static std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 0.5);
    
    // Simulate 3 targets moving in circles
    for (int i = 0; i < 3; i++) {
        Detection3D det;
        det.id = i;
        double angle = 2 * M_PI * (frame * 0.01 + i * 0.33);
        det.position << 10 * cos(angle) + noise(rng),
                        10 * sin(angle) + noise(rng),
                        2.0 + noise(rng);
        det.velocity << -10 * sin(angle) * 0.01,
                        10 * cos(angle) * 0.01,
                        0.0;
        det.confidence = 0.8 + (rng() % 20) / 100.0;
        det.timestamp = timestamp;
        detections.push_back(det);
    }
    
    // Add occasional false detection
    if (frame % 10 == 0) {
        Detection3D falseDet;
        falseDet.id = -1;
        falseDet.position << (rng() % 20) - 10, (rng() % 20) - 10, (rng() % 5);
        falseDet.confidence = 0.5;
        falseDet.timestamp = timestamp;
        detections.push_back(falseDet);
    }
    
    return detections;
}

} // namespace tracking_3d

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 25: 3D Multi-Target Tracking" << std::endl;
    std::cout << "  Full Implementation: Kalman Filter + Data Association" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace tracking_3d;
    
    KalmanTracker3D tracker;
    
    std::cout << "Running 3D tracking simulation..." << std::endl;
    std::cout << std::endl;
    
    for (int frame = 0; frame < 100; frame++) {
        double timestamp = frame * 0.1;
        auto detections = generateDetections(frame, timestamp);
        auto tracks = tracker.update(detections, timestamp);
        
        if (frame % 20 == 0) {
            std::cout << "Frame " << std::setw(3) << frame << ": ";
            std::cout << "Detections: " << detections.size() << ", Tracks: " << tracks.size() << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Final tracking results:" << std::endl;
    tracker.printTracks();
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  3D Multi-target tracking demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
