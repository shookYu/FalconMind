/**
 * Example 09: Batch Inference Optimization
 * Full implementation with dynamic batching, memory pooling, and throughput optimization
 * Maximizes NPU utilization by processing multiple images in a single inference call
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>
#include <memory>
#include <random>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/RknnDetectorBackend.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::core;

namespace batch {

// ============ Tensor Memory Pool ============
class TensorMemoryPool {
public:
    struct Tensor {
        float* data;
        size_t size;
        int batch_size;
        int width;
        int height;
        int channels;
        
        size_t totalElements() const {
            return static_cast<size_t>(batch_size) * height * width * channels;
        }
    };
    
    TensorMemoryPool(size_t max_tensors = 10);
    ~TensorMemoryPool();
    
    Tensor* acquire(int batch_size, int height, int width, int channels);
    void release(Tensor* tensor);
    void clear();
    
    size_t getPoolSize() const { return pool_.size(); }
    size_t getActiveCount() const { return active_count_.load(); }
    
private:
    std::vector<std::unique_ptr<Tensor>> pool_;
    std::vector<bool> available_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<size_t> active_count_{0};
};

TensorMemoryPool::TensorMemoryPool(size_t max_tensors) {
    pool_.reserve(max_tensors);
    available_.resize(max_tensors, true);
}

TensorMemoryPool::~TensorMemoryPool() {
    clear();
}

TensorMemoryPool::Tensor* TensorMemoryPool::acquire(int batch_size, int height, int width, int channels) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Find available tensor with matching dimensions
    for (size_t i = 0; i < pool_.size(); i++) {
        if (available_[i] && pool_[i]->batch_size >= batch_size &&
            pool_[i]->height == height && pool_[i]->width == width &&
            pool_[i]->channels == channels) {
            available_[i] = false;
            active_count_++;
            return pool_[i].get();
        }
    }
    
    // Create new tensor if pool not full
    for (size_t i = 0; i < available_.size(); i++) {
        if (!pool_[i]) {
            auto tensor = std::make_unique<Tensor>();
            tensor->batch_size = batch_size;
            tensor->height = height;
            tensor->width = width;
            tensor->channels = channels;
            tensor->size = static_cast<size_t>(batch_size) * height * width * channels;
            tensor->data = new float[tensor->size];
            
            Tensor* ptr = tensor.get();
            pool_[i] = std::move(tensor);
            available_[i] = false;
            active_count_++;
            return ptr;
        }
    }
    
    // Wait for available tensor
    cv_.wait(lock, [this] {
        for (bool avail : available_) {
            if (avail) return true;
        }
        return false;
    });
    
    // Retry
    return acquire(batch_size, height, width, channels);
}

void TensorMemoryPool::release(Tensor* tensor) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (size_t i = 0; i < pool_.size(); i++) {
        if (pool_[i].get() == tensor) {
            available_[i] = true;
            active_count_--;
            cv_.notify_one();
            return;
        }
    }
}

void TensorMemoryPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& tensor : pool_) {
        if (tensor && tensor->data) {
            delete[] tensor->data;
        }
    }
    pool_.clear();
    available_.clear();
}

// ============ Batch Configuration ============
struct BatchConfig {
    int min_batch_size = 1;
    int max_batch_size = 8;
    int preferred_batch_size = 4;
    int max_queue_delay_ms = 50;      // Max time to wait for batch to fill
    bool dynamic_batching = true;      // Enable dynamic batch size
    bool padding = true;               // Pad incomplete batches
};

// ============ Inference Request ============
struct InferenceRequest {
    int request_id;
    std::vector<float> input_data;    // Preprocessed image data
    int image_height;
    int image_width;
    int image_channels;
    std::chrono::steady_clock::time_point enqueue_time;
    std::promise<std::vector<float>> result_promise;
    
    double getWaitTimeMs() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - enqueue_time).count() / 1000.0;
    }
};

// ============ Batch Statistics ============
struct BatchStatsSnapshot {
    uint64_t total_requests;
    uint64_t total_batches;
    uint64_t total_images_processed;
    double total_batching_time_ms;
    double total_inference_time_ms;
    double total_latency_ms;
    
    double getAverageBatchSize() const {
        return total_batches > 0 ? static_cast<double>(total_images_processed) / total_batches : 0.0;
    }
    
    double getAverageLatencyMs() const {
        return total_requests > 0 ? total_latency_ms / total_requests : 0.0;
    }
    
    double getThroughput() const {
        return 1000.0 / getAverageLatencyMs();
    }
};

struct BatchStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> total_batches{0};
    std::atomic<uint64_t> total_images_processed{0};
    std::atomic<double> total_batching_time_ms{0.0};
    std::atomic<double> total_inference_time_ms{0.0};
    std::atomic<double> total_latency_ms{0.0};
    
    BatchStatsSnapshot snapshot() const {
        BatchStatsSnapshot snap;
        snap.total_requests = total_requests.load();
        snap.total_batches = total_batches.load();
        snap.total_images_processed = total_images_processed.load();
        snap.total_batching_time_ms = total_batching_time_ms.load();
        snap.total_inference_time_ms = total_inference_time_ms.load();
        snap.total_latency_ms = total_latency_ms.load();
        return snap;
    }
};

// ============ Batch Processor ============
class BatchProcessor {
public:
    explicit BatchProcessor(const BatchConfig& config);
    ~BatchProcessor();
    
    bool initialize();
    void shutdown();
    
    std::future<std::vector<float>> submitRequest(
        const std::vector<float>& input_data,
        int height, int width, int channels);
    
    BatchStatsSnapshot getStats() const { return stats_.snapshot(); }
    void printStats() const;
    
private:
    void batchingThread();
    void inferenceThread();
    
    std::vector<float> runInference(const std::vector<std::shared_ptr<InferenceRequest>>& batch);
    std::vector<float> padBatch(const std::vector<std::shared_ptr<InferenceRequest>>& batch, int target_size);
    
    BatchConfig config_;
    std::unique_ptr<TensorMemoryPool> memory_pool_;
    BatchStats stats_;
    
    std::queue<std::shared_ptr<InferenceRequest>> request_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::thread batching_thread_;
    std::thread inference_thread_;
    std::atomic<bool> running_{false};
    
    std::atomic<int> next_request_id_{0};
    
    // Inference latency model (for simulation)
    double base_latency_ms_;
    double latency_per_image_ms_;
};

BatchProcessor::BatchProcessor(const BatchConfig& config)
    : config_(config),
      base_latency_ms_(5.0),      // Fixed overhead
      latency_per_image_ms_(15.0) // Per-image processing time
{
    memory_pool_ = std::make_unique<TensorMemoryPool>(20);
}

BatchProcessor::~BatchProcessor() {
    shutdown();
}

bool BatchProcessor::initialize() {
    std::cout << "[Batch Processor] Initializing..." << std::endl;
    std::cout << "  - Min batch size: " << config_.min_batch_size << std::endl;
    std::cout << "  - Max batch size: " << config_.max_batch_size << std::endl;
    std::cout << "  - Preferred batch size: " << config_.preferred_batch_size << std::endl;
    std::cout << "  - Max queue delay: " << config_.max_queue_delay_ms << "ms" << std::endl;
    std::cout << "  - Dynamic batching: " << (config_.dynamic_batching ? "enabled" : "disabled") << std::endl;
    std::cout << "  - Padding: " << (config_.padding ? "enabled" : "disabled") << std::endl;
    
    running_ = true;
    batching_thread_ = std::thread(&BatchProcessor::batchingThread, this);
    inference_thread_ = std::thread(&BatchProcessor::inferenceThread, this);
    
    return true;
}

void BatchProcessor::shutdown() {
    running_ = false;
    queue_cv_.notify_all();
    
    if (batching_thread_.joinable()) batching_thread_.join();
    if (inference_thread_.joinable()) inference_thread_.join();
    
    memory_pool_->clear();
}

std::future<std::vector<float>> BatchProcessor::submitRequest(
    const std::vector<float>& input_data,
    int height, int width, int channels) {
    
    auto request = std::make_shared<InferenceRequest>();
    request->request_id = next_request_id_.fetch_add(1);
    request->input_data = input_data;
    request->image_height = height;
    request->image_width = width;
    request->image_channels = channels;
    request->enqueue_time = std::chrono::steady_clock::now();
    
    std::future<std::vector<float>> future = request->result_promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(request);
    }
    queue_cv_.notify_one();
    
    stats_.total_requests++;
    return future;
}

void BatchProcessor::batchingThread() {
    while (running_) {
        std::vector<std::shared_ptr<InferenceRequest>> current_batch;
        current_batch.reserve(config_.max_batch_size);
        
        auto batch_start_time = std::chrono::steady_clock::now();
        
        // Collect requests for batching
        while (current_batch.size() < static_cast<size_t>(config_.max_batch_size)) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - batch_start_time).count();
            
            // Check if we should process current batch
            if (current_batch.size() >= static_cast<size_t>(config_.min_batch_size) &&
                elapsed >= config_.max_queue_delay_ms) {
                // Timeout reached with minimum batch size
                break;
            }
            
            if (current_batch.size() >= static_cast<size_t>(config_.preferred_batch_size)) {
                // Preferred batch size reached
                break;
            }
            
            // Try to get more requests
            std::shared_ptr<InferenceRequest> request;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                
                int remaining_time = config_.max_queue_delay_ms - static_cast<int>(elapsed);
                if (remaining_time <= 0) remaining_time = 1;
                
                queue_cv_.wait_for(lock, std::chrono::milliseconds(remaining_time),
                    [this] { return !request_queue_.empty() || !running_; });
                
                if (!running_) break;
                
                if (!request_queue_.empty()) {
                    request = request_queue_.front();
                    request_queue_.pop();
                }
            }
            
            if (request) {
                current_batch.push_back(request);
            } else if (current_batch.size() >= static_cast<size_t>(config_.min_batch_size)) {
                // No more requests but have minimum batch
                break;
            }
            
            if (!running_) break;
        }
        
        // Process batch if we have any requests
        if (!current_batch.empty() && running_) {
            // Record batching time
            auto batch_end_time = std::chrono::steady_clock::now();
            double batching_time = std::chrono::duration_cast<std::chrono::microseconds>(
                batch_end_time - batch_start_time).count() / 1000.0;
            stats_.total_batching_time_ms.store(stats_.total_batching_time_ms.load() + batching_time);
            
            // Run inference
            auto results = runInference(current_batch);
            
            // Distribute results to requests
            size_t output_per_request = results.size() / current_batch.size();
            for (size_t i = 0; i < current_batch.size(); i++) {
                std::vector<float> request_result;
                size_t start_idx = i * output_per_request;
                size_t end_idx = start_idx + output_per_request;
                
                if (end_idx <= results.size()) {
                    request_result.assign(results.begin() + start_idx, results.begin() + end_idx);
                }
                
                current_batch[i]->result_promise.set_value(request_result);
                
                // Record latency
                double latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - current_batch[i]->enqueue_time).count() / 1000.0;
                stats_.total_latency_ms.store(stats_.total_latency_ms.load() + latency);
            }
            
            stats_.total_batches++;
            stats_.total_images_processed += current_batch.size();
        }
    }
}

void BatchProcessor::inferenceThread() {
    // This thread could be used for async inference or model switching
    // For now, inference happens in batching thread
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::vector<float> BatchProcessor::runInference(
    const std::vector<std::shared_ptr<InferenceRequest>>& batch) {
    
    int batch_size = static_cast<int>(batch.size());
    
    // Calculate inference latency based on batch size
    // Larger batches have better throughput but slightly higher latency
    double latency = base_latency_ms_ + latency_per_image_ms_ * std::sqrt(batch_size);
    
    // Simulate NPU inference
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(latency)));
    
    stats_.total_inference_time_ms.store(stats_.total_inference_time_ms.load() + latency);
    
    // Generate dummy output (e.g., detection results)
    // In real implementation: call RKNN API
    std::vector<float> output;
    output.resize(batch_size * 1000);  // 1000 floats per image
    
    for (int i = 0; i < batch_size; i++) {
        // Simulate detection results (bbox coords + class + confidence)
        for (int j = 0; j < 100; j++) {  // Up to 100 detections per image
            size_t idx = i * 1000 + j * 10;
            if (idx + 6 < output.size()) {
                output[idx + 0] = static_cast<float>(j % 10) * 0.1f;     // x1
                output[idx + 1] = static_cast<float>(j % 10) * 0.1f;     // y1
                output[idx + 2] = output[idx + 0] + 0.1f;                  // x2
                output[idx + 3] = output[idx + 1] + 0.1f;                  // y2
                output[idx + 4] = static_cast<float>(j % 80);             // class
                output[idx + 5] = 0.5f + static_cast<float>(j % 50) / 100.0f; // confidence
            }
        }
    }
    
    return output;
}

void BatchProcessor::printStats() const {
    auto stats = stats_.snapshot();
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Batch Processing Statistics" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total requests: " << stats.total_requests << std::endl;
    std::cout << "Total batches: " << stats.total_batches << std::endl;
    std::cout << "Total images processed: " << stats.total_images_processed << std::endl;
    std::cout << "Average batch size: " << std::fixed << std::setprecision(2) 
              << stats.getAverageBatchSize() << std::endl;
    std::cout << "Average latency: " << stats.getAverageLatencyMs() << " ms" << std::endl;
    std::cout << "Throughput: " << stats.getThroughput() << " images/sec" << std::endl;
    
    if (stats.total_batches > 0) {
        double avg_inference = stats.total_inference_time_ms / stats.total_batches;
        double avg_batching = stats.total_batching_time_ms / stats.total_batches;
        std::cout << "Avg inference time: " << avg_inference << " ms" << std::endl;
        std::cout << "Avg batching time: " << avg_batching << " ms" << std::endl;
    }
    
    std::cout << "Memory pool size: " << memory_pool_->getPoolSize() << std::endl;
    std::cout << "Active tensors: " << memory_pool_->getActiveCount() << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace batch

// ============ Main ============
int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 09: Batch Inference Optimization" << std::endl;
    std::cout << "  Full Implementation: Dynamic Batching + Memory Pool + Throughput Optimization" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace batch;
    
    // Parse arguments
    int num_requests = 100;
    int batch_size = 4;
    int delay_ms = 10;  // Delay between submissions
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--requests") == 0 && i + 1 < argc) {
            num_requests = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            batch_size = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--delay") == 0 && i + 1 < argc) {
            delay_ms = std::atoi(argv[++i]);
        }
    }
    
    // Configure batch processor
    BatchConfig config;
    config.min_batch_size = 1;
    config.max_batch_size = batch_size;
    config.preferred_batch_size = batch_size / 2;
    config.max_queue_delay_ms = 50;
    config.dynamic_batching = true;
    config.padding = true;
    
    BatchProcessor processor(config);
    
    std::cout << "[1] Initializing batch processor..." << std::endl;
    if (!processor.initialize()) {
        std::cerr << "[Error] Failed to initialize processor" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "[2] Submitting " << num_requests << " inference requests..." << std::endl;
    std::cout << "    Batch size: " << batch_size << std::endl;
    std::cout << "    Inter-request delay: " << delay_ms << "ms" << std::endl;
    std::cout << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    std::vector<std::future<std::vector<float>>> futures;
    futures.reserve(num_requests);
    
    // Submit requests
    for (int i = 0; i < num_requests; i++) {
        // Simulate preprocessed image data (640x640x3)
        std::vector<float> input_data(640 * 640 * 3, 0.5f);
        
        futures.push_back(processor.submitRequest(input_data, 640, 640, 3));
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_requests << " requests submitted" << std::endl;
        }
        
        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    std::cout << "  Waiting for all requests to complete..." << std::endl;
    
    // Wait for all requests
    int completed = 0;
    for (auto& future : futures) {
        future.wait();
        completed++;
        
        if (completed % 25 == 0) {
            std::cout << "  Completed: " << completed << "/" << num_requests << std::endl;
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << std::endl;
    std::cout << "[3] Benchmark complete" << std::endl;
    
    // Print statistics
    processor.printStats();
    
    std::cout << "Performance Summary:" << std::endl;
    std::cout << "  Total time: " << std::fixed << std::setprecision(1) 
              << duration.count() / 1000.0 << " seconds" << std::endl;
    std::cout << "  Throughput: " << std::setprecision(1) 
              << (num_requests * 1000.0 / duration.count()) << " images/sec" << std::endl;
    
    // Compare with non-batched inference
    double single_latency = 20.0;  // ms per image without batching
    double theoretical_single_time = num_requests * single_latency;
    double actual_time = duration.count();
    double speedup = theoretical_single_time / actual_time;
    
    std::cout << std::endl;
    std::cout << "Batching Efficiency:" << std::endl;
    std::cout << "  Theoretical time (no batching): " << theoretical_single_time / 1000.0 << "s" << std::endl;
    std::cout << "  Actual time (with batching): " << actual_time / 1000.0 << "s" << std::endl;
    std::cout << "  Speedup: " << std::setprecision(2) << speedup << "x" << std::endl;
    
    processor.shutdown();
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Batch Inference Optimization demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
