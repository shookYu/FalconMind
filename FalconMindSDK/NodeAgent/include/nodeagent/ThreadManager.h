#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <vector>
#include <memory>
#include <future>
#include <chrono>

namespace nodeagent {

enum class ThreadPriority {
    IDLE = 0,
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    REALTIME = 4
};

enum class TaskType {
    HEARTBEAT_CHECK,
    RULE_EVALUATION,
    TELEMETRY_CACHE,
    SYNC_OPERATION,
    GENERAL
};

struct Task {
    uint64_t id;
    TaskType type;
    std::string name;
    std::function<void()> function;
    std::chrono::steady_clock::time_point scheduledTime;
    std::chrono::milliseconds interval;
    bool repeatable;
    bool cancelled;
    std::promise<void>* promise;
    
    Task() : id(0), type(TaskType::GENERAL), repeatable(false), cancelled(false), promise(nullptr) {}
};

class ThreadManager {
public:
    using TaskId = uint64_t;
    using ExceptionHandler = std::function<void(const std::exception&, const Task&)>;

    ThreadManager();
    ~ThreadManager();

    // Initialization
    bool initialize(size_t workerThreadCount = 4);
    void shutdown();
    bool isRunning() const;

    // Task submission
    TaskId submitTask(const std::function<void()>& task, TaskType type = TaskType::GENERAL);
    TaskId submitTask(const std::string& name, const std::function<void()>& task, 
                      TaskType type = TaskType::GENERAL);
    TaskId scheduleTask(const std::function<void()>& task, 
                        std::chrono::milliseconds delay,
                        TaskType type = TaskType::GENERAL);
    TaskId scheduleRepeatingTask(const std::function<void()>& task,
                                  std::chrono::milliseconds interval,
                                  TaskType type = TaskType::GENERAL);
    TaskId scheduleRepeatingTask(const std::string& name,
                                  const std::function<void()>& task,
                                  std::chrono::milliseconds interval,
                                  TaskType type = TaskType::GENERAL);

    // Task management
    bool cancelTask(TaskId taskId);
    bool isTaskRunning(TaskId taskId) const;
    bool waitForTask(TaskId taskId, std::chrono::milliseconds timeout);
    size_t getPendingTaskCount() const;
    size_t getActiveThreadCount() const;

    // Thread control
    void setWorkerThreadCount(size_t count);
    void setThreadPriority(ThreadPriority priority);
    void setCpuAffinity(const std::vector<int>& cpuCores);

    // Task type specific
    size_t cancelTasksByType(TaskType type);
    void pauseTasksByType(TaskType type);
    void resumeTasksByType(TaskType type);

    // Exception handling
    void setExceptionHandler(ExceptionHandler handler);

    // Statistics
    struct Statistics {
        size_t totalTasksSubmitted;
        size_t totalTasksExecuted;
        size_t totalTasksCancelled;
        size_t totalExceptions;
        std::chrono::milliseconds averageTaskExecutionTime;
    };
    Statistics getStatistics() const;

private:
    void workerThreadFunc(size_t threadId);
    void schedulerThreadFunc();
    void executeTask(const Task& task);
    TaskId generateTaskId();
    
    std::atomic<bool> running_;
    std::atomic<TaskId> nextTaskId_;
    
    std::vector<std::thread> workerThreads_;
    std::thread schedulerThread_;
    
    std::queue<Task> taskQueue_;
    mutable std::mutex taskQueueMutex_;
    std::condition_variable taskCondition_;
    
    std::priority_queue<Task, std::vector<Task>, 
                      std::function<bool(const Task&, const Task&)>> scheduledTasks_;
    mutable std::mutex scheduledTasksMutex_;
    std::condition_variable schedulerCondition_;
    
    std::unordered_map<TaskId, std::shared_ptr<std::atomic<bool>>> activeTasks_;
    mutable std::mutex activeTasksMutex_;
    
    std::unordered_set<TaskType> pausedTaskTypes_;
    mutable std::mutex pausedTypesMutex_;
    
    ExceptionHandler exceptionHandler_;
    
    Statistics stats_;
    mutable std::mutex statsMutex_;
};

} // namespace nodeagent
