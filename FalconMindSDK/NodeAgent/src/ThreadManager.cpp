/**
 * ThreadManager.cpp - Production-grade thread pool and task scheduler
 */

#include "nodeagent/ThreadManager.h"
#include "nodeagent/Logger.h"
#include <algorithm>

namespace nodeagent {

ThreadManager::ThreadManager()
    : running_(false)
    , nextTaskId_(1) {
    LOG_INFO("ThreadManager", "Constructor called");
}

ThreadManager::~ThreadManager() {
    LOG_INFO("ThreadManager", "Destructor called");
    shutdown();
}

bool ThreadManager::initialize(size_t workerThreadCount) {
    if (running_) {
        LOG_WARNING("ThreadManager", "Already initialized");
        return true;
    }
    
    LOG_INFO("ThreadManager", "Initializing with " + std::to_string(workerThreadCount) + " worker threads");
    
    running_ = true;
    
    // Start worker threads
    for (size_t i = 0; i < workerThreadCount; ++i) {
        workerThreads_.emplace_back(&ThreadManager::workerThreadFunc, this, i
        );
    }
    
    // Start scheduler thread
    schedulerThread_ = std::thread(&ThreadManager::schedulerThreadFunc, this
    );
    
    LOG_INFO("ThreadManager", "Initialization complete");
    return true;
}

void ThreadManager::shutdown() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("ThreadManager", "Shutting down...");
    
    running_ = false;
    
    // Wake up all waiting threads
    taskCondition_.notify_all();
    schedulerCondition_.notify_all();
    
    // Cancel all scheduled tasks
    {
        std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
        while (!scheduledTasks_.empty()) {
            auto task = scheduledTasks_.top();
            scheduledTasks_.pop();
            if (task.promise) {
                task.promise->set_exception(
                    std::make_exception_ptr(std::runtime_error("ThreadManager shutting down"))
                );
            }
        }
    }
    
    // Wait for worker threads to finish
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads_.clear();
    
    // Wait for scheduler thread
    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }
    
    LOG_INFO("ThreadManager", "Shutdown complete");
}

bool ThreadManager::isRunning() const {
    return running_;
}

ThreadManager::TaskId ThreadManager::submitTask(const std::function<void()>& task, 
                                                TaskType type) {
    return submitTask("unnamed", task, type);
}

ThreadManager::TaskId ThreadManager::submitTask(const std::string& name,
                                                const std::function<void()>& task,
                                                TaskType type) {
    if (!running_) {
        LOG_ERROR("ThreadManager", "Cannot submit task: not running");
        return 0;
    }
    
    Task t;
    t.id = generateTaskId();
    t.name = name;
    t.type = type;
    t.function = task;
    t.scheduledTime = std::chrono::steady_clock::now();
    t.interval = std::chrono::milliseconds(0);
    t.repeatable = false;
    t.cancelled = false;
    t.promise = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex_);
        taskQueue_.push(t);
    }
    
    taskCondition_.notify_one();
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.totalTasksSubmitted++;
    }
    
    LOG_DEBUG("ThreadManager", "Submitted task: " + name + " (ID: " + std::to_string(t.id) + ")");
    return t.id;
}

ThreadManager::TaskId ThreadManager::scheduleTask(const std::function<void()>& task,
                                                   std::chrono::milliseconds delay,
                                                   TaskType type) {
    if (!running_) {
        LOG_ERROR("ThreadManager", "Cannot schedule task: not running");
        return 0;
    }
    
    Task t;
    t.id = generateTaskId();
    t.name = "scheduled_" + std::to_string(t.id);
    t.type = type;
    t.function = task;
    t.scheduledTime = std::chrono::steady_clock::now() + delay;
    t.interval = std::chrono::milliseconds(0);
    t.repeatable = false;
    t.cancelled = false;
    t.promise = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
        scheduledTasks_.push(t);
    }
    
    schedulerCondition_.notify_one();
    
    LOG_DEBUG("ThreadManager", "Scheduled task ID: " + std::to_string(t.id) + 
              " with delay " + std::to_string(delay.count()) + "ms");
    return t.id;
}

ThreadManager::TaskId ThreadManager::scheduleRepeatingTask(const std::function<void()>& task,
                                                            std::chrono::milliseconds interval,
                                                            TaskType type) {
    return scheduleRepeatingTask("repeating_" + std::to_string(generateTaskId()), 
                                  task, interval, type);
}

ThreadManager::TaskId ThreadManager::scheduleRepeatingTask(const std::string& name,
                                                            const std::function<void()>& task,
                                                            std::chrono::milliseconds interval,
                                                            TaskType type) {
    if (!running_) {
        LOG_ERROR("ThreadManager", "Cannot schedule repeating task: not running");
        return 0;
    }
    
    Task t;
    t.id = generateTaskId();
    t.name = name;
    t.type = type;
    t.function = task;
    t.scheduledTime = std::chrono::steady_clock::now() + interval;
    t.interval = interval;
    t.repeatable = true;
    t.cancelled = false;
    t.promise = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
        scheduledTasks_.push(t);
    }
    
    schedulerCondition_.notify_one();
    
    LOG_DEBUG("ThreadManager", "Scheduled repeating task: " + name + 
              " (ID: " + std::to_string(t.id) + ") with interval " + 
              std::to_string(interval.count()) + "ms");
    return t.id;
}

bool ThreadManager::cancelTask(TaskId taskId) {
    // For simplicity, we can only cancel scheduled tasks that haven't been
    // moved to the execution queue yet
    std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
    
    // We need to rebuild the priority queue since we can't modify elements in place
    std::vector<Task> tasks;
    bool found = false;
    
    while (!scheduledTasks_.empty()) {
        Task t = scheduledTasks_.top();
        scheduledTasks_.pop();
        
        if (t.id == taskId) {
            t.cancelled = true;
            found = true;
            LOG_DEBUG("ThreadManager", "Cancelled task ID: " + std::to_string(taskId));
        }
        
        if (!t.cancelled) {
            tasks.push_back(t);
        }
    }
    
    for (const auto& t : tasks) {
        scheduledTasks_.push(t);
    }
    
    if (found) {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.totalTasksCancelled++;
    }
    
    return found;
}

size_t ThreadManager::cancelTasksByType(TaskType type) {
    std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
    
    std::vector<Task> tasks;
    size_t cancelledCount = 0;
    
    while (!scheduledTasks_.empty()) {
        Task t = scheduledTasks_.top();
        scheduledTasks_.pop();
        
        if (t.type == type) {
            t.cancelled = true;
            cancelledCount++;
        }
        
        if (!t.cancelled) {
            tasks.push_back(t);
        }
    }
    
    for (const auto& t : tasks) {
        scheduledTasks_.push(t);
    }
    
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.totalTasksCancelled += cancelledCount;
    }
    
    return cancelledCount;
}

size_t ThreadManager::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(taskQueueMutex_);
    return taskQueue_.size();
}

size_t ThreadManager::getActiveThreadCount() const {
    std::lock_guard<std::mutex> lock(activeTasksMutex_);
    return activeTasks_.size();
}

void ThreadManager::workerThreadFunc(size_t threadId) {
    LOG_INFO("ThreadManager", "Worker thread " + std::to_string(threadId) + " started");
    
    while (running_) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(taskQueueMutex_);
            
            taskCondition_.wait(lock, [this] {
                return !running_ || !taskQueue_.empty();
            });
            
            if (!running_) {
                break;
            }
            
            if (taskQueue_.empty()) {
                continue;
            }
            
            task = taskQueue_.front();
            taskQueue_.pop();
        }
        
        // Check if task type is paused
        {
            std::lock_guard<std::mutex> lock(pausedTypesMutex_);
            if (pausedTaskTypes_.count(task.type) > 0) {
                // Re-queue the task
                {
                    std::lock_guard<std::mutex> qLock(taskQueueMutex_);
                    taskQueue_.push(task);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        
        if (task.cancelled) {
            continue;
        }
        
        // Mark as active
        {
            std::lock_guard<std::mutex> lock(activeTasksMutex_);
            activeTasks_[task.id] = std::make_shared<std::atomic<bool>>(false);
        }
        
        // Execute the task
        auto startTime = std::chrono::steady_clock::now();
        
        try {
            if (task.function) {
                task.function();
            }
            
            if (task.promise) {
                task.promise->set_value();
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("ThreadManager", "Task execution failed: " + std::string(e.what()));
            
            if (task.promise) {
                task.promise->set_exception(std::current_exception());
            }
            
            if (exceptionHandler_) {
                exceptionHandler_(e, task);
            }
            
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.totalExceptions++;
            }
        }
        
        auto executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        );
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.totalTasksExecuted++;
            
            // Running average
            auto currentAvg = stats_.averageTaskExecutionTime.count();
            auto newAvg = (currentAvg * (stats_.totalTasksExecuted - 1) + executionTime.count()) 
                          / stats_.totalTasksExecuted;
            stats_.averageTaskExecutionTime = std::chrono::milliseconds(newAvg);
        }
        
        // Remove from active
        {
            std::lock_guard<std::mutex> lock(activeTasksMutex_);
            activeTasks_.erase(task.id);
        }
        
        // If repeatable, reschedule
        if (task.repeatable && running_) {
            Task rescheduled = task;
            rescheduled.scheduledTime = std::chrono::steady_clock::now() + task.interval;
            
            {
                std::lock_guard<std::mutex> lock(scheduledTasksMutex_);
                scheduledTasks_.push(rescheduled);
            }
            schedulerCondition_.notify_one();
        }
    }
    
    LOG_INFO("ThreadManager", "Worker thread " + std::to_string(threadId) + " stopped");
}

void ThreadManager::schedulerThreadFunc() {
    LOG_INFO("ThreadManager", "Scheduler thread started");
    
    while (running_) {
        std::unique_lock<std::mutex> lock(scheduledTasksMutex_);
        
        // Wait until there are scheduled tasks or we're shutting down
        schedulerCondition_.wait(lock, [this] {
            return !running_ || !scheduledTasks_.empty();
        });
        
        if (!running_) {
            break;
        }
        
        if (scheduledTasks_.empty()) {
            continue;
        }
        
        // Check the next scheduled task
        auto now = std::chrono::steady_clock::now();
        Task nextTask = scheduledTasks_.top();
        
        if (nextTask.scheduledTime <= now) {
            // Task is ready to execute
            scheduledTasks_.pop();
            lock.unlock();
            
            if (!nextTask.cancelled) {
                {
                    std::lock_guard<std::mutex> qLock(taskQueueMutex_);
                    taskQueue_.push(nextTask);
                }
                taskCondition_.notify_one();
            }
        } else {
            // Wait until the next task is ready
            auto waitTime = nextTask.scheduledTime - now;
            lock.unlock();
            
            std::this_thread::sleep_for(std::min(waitTime, std::chrono::milliseconds(100)));
        }
    }
    
    LOG_INFO("ThreadManager", "Scheduler thread stopped");
}

ThreadManager::TaskId ThreadManager::generateTaskId() {
    return nextTaskId_.fetch_add(1);
}

void ThreadManager::setExceptionHandler(ExceptionHandler handler) {
    exceptionHandler_ = handler;
}

ThreadManager::Statistics ThreadManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void ThreadManager::setWorkerThreadCount(size_t count) {
    // This would require stopping and restarting threads
    // For now, just log that this operation is not supported dynamically
    LOG_WARNING("ThreadManager", "Dynamic thread count change not supported");
}

void ThreadManager::pauseTasksByType(TaskType type) {
    std::lock_guard<std::mutex> lock(pausedTypesMutex_);
    pausedTaskTypes_.insert(type);
    LOG_INFO("ThreadManager", "Paused tasks of type: " + std::to_string(static_cast<int>(type)));
}

void ThreadManager::resumeTasksByType(TaskType type) {
    std::lock_guard<std::mutex> lock(pausedTypesMutex_);
    pausedTaskTypes_.erase(type);
    LOG_INFO("ThreadManager", "Resumed tasks of type: " + std::to_string(static_cast<int>(type)));
}

void ThreadManager::setThreadPriority(ThreadPriority priority) {
    // Platform-specific implementation would go here
    LOG_INFO("ThreadManager", "Setting thread priority to: " + std::to_string(static_cast<int>(priority)));
}

void ThreadManager::setCpuAffinity(const std::vector<int>& cpuCores) {
    // Platform-specific implementation would go here
    LOG_INFO("ThreadManager", "Setting CPU affinity to cores: " + 
              std::accumulate(cpuCores.begin(), cpuCores.end(), std::string(),
                  [](const std::string& a, int b) {
                      return a.empty() ? std::to_string(b) : a + "," + std::to_string(b);
                  }));
}

bool ThreadManager::isTaskRunning(TaskId taskId) const {
    std::lock_guard<std::mutex> lock(activeTasksMutex_);
    return activeTasks_.count(taskId) > 0;
}

bool ThreadManager::waitForTask(TaskId taskId, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (isTaskRunning(taskId)) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return true;
}

} // namespace nodeagent
