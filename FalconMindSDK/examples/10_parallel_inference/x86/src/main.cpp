/**
 * Example 10: Parallel Multi-Model Inference
 * Full implementation running multiple models (YOLO + Classification + Segmentation) in parallel
 * Maximizes throughput by executing independent models concurrently
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
#include <map>
#include <random>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::core;

namespace parallel {

// ============ Model Types ============
enum class ModelType {
    ObjectDetection,      // YOLO-style detection
    ImageClassification,  // ResNet-style classification
    SemanticSegmentation, // Segmentation model
    InstanceSegmentation, // Mask R-CNN style
    DepthEstimation       // Monocular depth
};

struct ModelConfig {
    std::string name;
    ModelType type;
    int input_width;
    int input_height;
    int input_channels;
    double latency_ms;        // Simulated inference latency
    int priority;             // Execution priority (higher = more important)
};

// ============ Inference Result ============
struct Detection {
    float x1, y1, x2, y2;
    int class_id;
    float confidence;
};

struct Classification {
    int class_id;
    float confidence;
    std::vector<float> class_probs;
};

struct SegmentationMask {
    int width, height;
    std::vector<uint8_t> class_ids;
};

struct ModelOutput {
    std::string model_name;
    double inference_time_ms;
    std::vector<Detection> detections;
    Classification classification;
    SegmentationMask segmentation;
    std::vector<float> depth_map;
};

// ============ Inference Request ============
struct InferenceRequest {
    int request_id;
    std::vector<float> input_data;
    int width, height, channels;
    std::vector<std::string> target_models;
    std::chrono::steady_clock::time_point submit_time;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    std::map<std::string, ModelOutput> results;
    std::atomic<int> pending_models{0};
    std::promise<bool> completion_promise;
    
    double getTotalLatencyMs() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - submit_time).count() / 1000.0;
    }
};

// ============ Thread Pool ============
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<decltype(f(args...))>;
    
    size_t getQueueSize() const;
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};

ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    
                    if (stop_ && tasks_.empty()) return;
                    
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();
    for (auto& worker : workers_) {
        worker.join();
    }
}

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();
    return result;
}

size_t ThreadPool::getQueueSize() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

// ============ Model Runner ============
class ModelRunner {
public:
    explicit ModelRunner(const ModelConfig& config);
    
    ModelOutput run(const std::vector<float>& input_data, int width, int height, int channels);
    const ModelConfig& getConfig() const { return config_; }
    
private:
    ModelConfig config_;
    std::atomic<uint64_t> inference_count_{0};
};

ModelRunner::ModelRunner(const ModelConfig& config) : config_(config) {}

ModelOutput ModelRunner::run(const std::vector<float>& input_data, int width, int height, int channels) {
    auto start = std::chrono::steady_clock::now();
    
    // Simulate inference latency
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(config_.latency_ms)));
    
    ModelOutput output;
    output.model_name = config_.name;
    
    // Generate simulated output based on model type
    switch (config_.type) {
        case ModelType::ObjectDetection: {
            // Generate 5-15 random detections
            std::mt19937 rng(inference_count_.load());
            std::uniform_int_distribution<int> num_dets(5, 15);
            std::uniform_real_distribution<float> coord(0.0f, 1.0f);
            
            int num = num_dets(rng);
            for (int i = 0; i < num; i++) {
                Detection det;
                det.x1 = coord(rng) * 0.8f;
                det.y1 = coord(rng) * 0.8f;
                det.x2 = det.x1 + coord(rng) * 0.2f;
                det.y2 = det.y1 + coord(rng) * 0.2f;
                det.class_id = i % 80;
                det.confidence = 0.5f + coord(rng) * 0.5f;
                output.detections.push_back(det);
            }
            break;
        }
        
        case ModelType::ImageClassification: {
            output.classification.class_probs.resize(1000);
            float sum = 0.0f;
            for (int i = 0; i < 1000; i++) {
                output.classification.class_probs[i] = static_cast<float>(rand()) / RAND_MAX;
                sum += output.classification.class_probs[i];
            }
            // Normalize
            for (auto& p : output.classification.class_probs) p /= sum;
            
            // Find max
            auto max_it = std::max_element(output.classification.class_probs.begin(),
                                           output.classification.class_probs.end());
            output.classification.class_id = static_cast<int>(std::distance(
                output.classification.class_probs.begin(), max_it));
            output.classification.confidence = *max_it;
            break;
        }
        
        case ModelType::SemanticSegmentation: {
            output.segmentation.width = width / 4;
            output.segmentation.height = height / 4;
            output.segmentation.class_ids.resize(output.segmentation.width * output.segmentation.height);
            for (auto& id : output.segmentation.class_ids) {
                id = static_cast<uint8_t>(rand() % 21);  // 21 classes
            }
            break;
        }
        
        case ModelType::DepthEstimation: {
            output.depth_map.resize(width * height);
            for (auto& d : output.depth_map) {
                d = static_cast<float>(rand()) / RAND_MAX * 100.0f;  // 0-100m
            }
            break;
        }
        
        default:
            break;
    }
    
    auto end = std::chrono::steady_clock::now();
    output.inference_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count() / 1000.0;
    
    inference_count_++;
    return output;
}

// ============ Multi-Model Inference Engine ============
class MultiModelInferenceEngine {
public:
    MultiModelInferenceEngine();
    ~MultiModelInferenceEngine();
    
    bool initialize();
    void shutdown();
    
    void registerModel(const ModelConfig& config);
    std::future<bool> inferenceAsync(const std::vector<float>& input_data,
                                         int width, int height, int channels,
                                         const std::vector<std::string>& model_names);
    
    // Sequential baseline for comparison
    std::map<std::string, ModelOutput> inferenceSequential(
        const std::vector<float>& input_data,
        int width, int height, int channels,
        const std::vector<std::string>& model_names);
    
    void printStats() const;
    
private:
    void processRequest(std::shared_ptr<InferenceRequest> request);
    
    std::map<std::string, std::unique_ptr<ModelRunner>> models_;
    std::unique_ptr<ThreadPool> thread_pool_;
    
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_parallel_time_ms_{0};
    std::atomic<uint64_t> total_sequential_time_ms_{0};
    
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<bool> running_{false};
};

MultiModelInferenceEngine::MultiModelInferenceEngine() {}

MultiModelInferenceEngine::~MultiModelInferenceEngine() {
    shutdown();
}

bool MultiModelInferenceEngine::initialize() {
    std::cout << "[Multi-Model Engine] Initializing..." << std::endl;
    
    // Create thread pool (one thread per model is ideal)
    thread_pool_ = std::make_unique<ThreadPool>(4);
    
    start_time_ = std::chrono::steady_clock::now();
    running_ = true;
    
    std::cout << "[Multi-Model Engine] Initialized with thread pool (4 threads)" << std::endl;
    return true;
}

void MultiModelInferenceEngine::shutdown() {
    running_ = false;
    thread_pool_.reset();
    models_.clear();
}

void MultiModelInferenceEngine::registerModel(const ModelConfig& config) {
    models_[config.name] = std::make_unique<ModelRunner>(config);
    std::cout << "  Registered model: " << config.name << std::endl;
    std::cout << "    Type: " << static_cast<int>(config.type) << std::endl;
    std::cout << "    Input: " << config.input_width << "x" << config.input_height << std::endl;
    std::cout << "    Latency: " << config.latency_ms << "ms" << std::endl;
}

std::future<bool> MultiModelInferenceEngine::inferenceAsync(
    const std::vector<float>& input_data,
    int width, int height, int channels,
    const std::vector<std::string>& model_names) {
    
    auto request = std::make_shared<InferenceRequest>();
    request->request_id = static_cast<int>(total_requests_.fetch_add(1));
    request->input_data = input_data;
    request->width = width;
    request->height = height;
    request->channels = channels;
    request->target_models = model_names;
    request->submit_time = std::chrono::steady_clock::now();
    request->pending_models = static_cast<int>(model_names.size());
    
    std::future<bool> future = request->completion_promise.get_future();
    
    // Submit models in parallel
    for (const auto& model_name : model_names) {
        thread_pool_->enqueue([this, request, model_name]() {
            if (models_.count(model_name)) {
                auto output = models_[model_name]->run(
                    request->input_data, request->width, request->height, request->channels);
                
                request->results[model_name] = output;
            }
            
            if (request->pending_models.fetch_sub(1) == 1) {
                // Last model completed
                request->end_time = std::chrono::steady_clock::now();
                request->completion_promise.set_value(true);
            }
        });
    }
    
    return future;
}

std::map<std::string, ModelOutput> MultiModelInferenceEngine::inferenceSequential(
    const std::vector<float>& input_data,
    int width, int height, int channels,
    const std::vector<std::string>& model_names) {
    
    std::map<std::string, ModelOutput> results;
    
    for (const auto& model_name : model_names) {
        if (models_.count(model_name)) {
            results[model_name] = models_[model_name]->run(input_data, width, height, channels);
        }
    }
    
    return results;
}

void MultiModelInferenceEngine::printStats() const {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Multi-Model Inference Statistics" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total requests: " << total_requests_.load() << std::endl;
    std::cout << "Registered models: " << models_.size() << std::endl;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    
    if (elapsed > 0) {
        double throughput = total_requests_.load() / static_cast<double>(elapsed);
        std::cout << "Throughput: " << throughput << " requests/sec" << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
}

} // namespace parallel

// ============ Main ============
int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 10: Parallel Multi-Model Inference" << std::endl;
    std::cout << "  Full Implementation: Thread Pool + Concurrent Model Execution" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace parallel;
    
    // Parse arguments
    int num_requests = 50;
    bool run_sequential = false;
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--requests") == 0 && i + 1 < argc) {
            num_requests = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--sequential") == 0) {
            run_sequential = true;
        }
    }
    
    // Create engine
    MultiModelInferenceEngine engine;
    
    std::cout << "[1] Initializing multi-model inference engine..." << std::endl;
    if (!engine.initialize()) {
        std::cerr << "[Error] Failed to initialize engine" << std::endl;
        return 1;
    }
    
    // Register models
    std::cout << "[2] Registering models..." << std::endl;
    engine.registerModel({"yolo_detection", ModelType::ObjectDetection, 640, 640, 3, 25.0, 10});
    engine.registerModel({"resnet_classification", ModelType::ImageClassification, 224, 224, 3, 15.0, 5});
    engine.registerModel({"segmentation", ModelType::SemanticSegmentation, 512, 512, 3, 35.0, 3});
    engine.registerModel({"depth_estimation", ModelType::DepthEstimation, 640, 480, 3, 20.0, 2});
    
    std::cout << std::endl;
    
    // Models to run
    std::vector<std::string> models_to_run = {
        "yolo_detection",
        "resnet_classification",
        "segmentation",
        "depth_estimation"
    };
    
    std::cout << "[3] Running " << num_requests << " inference requests..." << std::endl;
    std::cout << "    Models per request: " << models_to_run.size() << std::endl;
    std::cout << "    Mode: " << (run_sequential ? "Sequential (baseline)" : "Parallel") << std::endl;
    std::cout << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    if (run_sequential) {
        // Sequential execution (baseline)
        for (int i = 0; i < num_requests; i++) {
            std::vector<float> input_data(640 * 640 * 3, 0.5f);
            engine.inferenceSequential(input_data, 640, 640, 3, models_to_run);
            
            if ((i + 1) % 10 == 0) {
                std::cout << "  Progress: " << (i + 1) << "/" << num_requests << std::endl;
            }
        }
    } else {
        // Parallel execution
        std::vector<std::future<bool>> futures;
        futures.reserve(num_requests);
        
        for (int i = 0; i < num_requests; i++) {
            std::vector<float> input_data(640 * 640 * 3, 0.5f);
            futures.push_back(engine.inferenceAsync(input_data, 640, 640, 3, models_to_run));
            
            if ((i + 1) % 10 == 0) {
                std::cout << "  Submitted: " << (i + 1) << "/" << num_requests << std::endl;
            }
        }
        
        std::cout << "  Waiting for completion..." << std::endl;
        
        // Wait for all
        int completed = 0;
        for (auto& future : futures) {
            future.wait();
            completed++;
            if (completed % 25 == 0) {
                std::cout << "  Completed: " << completed << "/" << num_requests << std::endl;
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << std::endl;
    std::cout << "[4] Benchmark complete" << std::endl;
    
    // Calculate theoretical times
    double sequential_time_per_request = 25.0 + 15.0 + 35.0 + 20.0;  // Sum of latencies
    double theoretical_sequential_total = num_requests * sequential_time_per_request;
    double theoretical_parallel_total = num_requests * 35.0;  // Max latency
    
    std::cout << std::endl;
    std::cout << "Performance Results:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(1) 
              << duration.count() / 1000.0 << " seconds" << std::endl;
    std::cout << "Throughput: " << std::setprecision(1) 
              << (num_requests * 1000.0 / duration.count()) << " requests/sec" << std::endl;
    std::cout << "Avg latency: " << std::setprecision(1) 
              << (duration.count() / static_cast<double>(num_requests)) << " ms/request" << std::endl;
    
    if (!run_sequential) {
        double speedup = theoretical_sequential_total / duration.count();
        std::cout << std::endl;
        std::cout << "Parallel Speedup:" << std::endl;
        std::cout << "  Theoretical sequential: " << theoretical_sequential_total / 1000.0 << "s" << std::endl;
        std::cout << "  Actual parallel: " << duration.count() / 1000.0 << "s" << std::endl;
        std::cout << "  Speedup: " << std::setprecision(2) << speedup << "x" << std::endl;
        std::cout << "  Efficiency: " << std::setprecision(1) 
                  << (speedup / models_to_run.size() * 100) << "%" << std::endl;
    }
    
    engine.printStats();
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Parallel Multi-Model Inference demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
