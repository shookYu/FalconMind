#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <mutex>
#include <chrono>

namespace nodeagent {

enum class StoreError {
    NONE = 0,
    DATABASE_NOT_OPEN,
    SQL_EXECUTION_FAILED,
    TRANSACTION_FAILED,
    INVALID_DATA,
    CONSTRAINT_VIOLATION,
    IO_ERROR,
    UNKNOWN
};

struct StoreResult {
    bool success;
    StoreError error;
    std::string message;
    
    StoreResult() : success(true), error(StoreError::NONE) {}
    StoreResult(bool s, StoreError e, const std::string& m) 
        : success(s), error(e), message(m) {}
    
    operator bool() const { return success; }
};

struct OfflineTask {
    std::string taskId;
    std::string uavId;
    std::string taskType;
    nlohmann::json missionData;
    nlohmann::json offlineRules;
    std::string status;
    std::string deployedAt;
    std::string activatedAt;
    std::string completedAt;
    int retryCount;
    nlohmann::json executionLog;
};

struct TelemetryRecord {
    int64_t id;
    std::string timestamp;
    std::string uavId;
    double latitude;
    double longitude;
    double altitude;
    int battery;
    std::string status;
    nlohmann::json extendedData;
    bool synced;
    int64_t sequenceNum;
};

struct OfflineEvent {
    int64_t id;
    std::string timestamp;
    std::string uavId;
    std::string eventType;
    std::string severity;
    nlohmann::json data;
    bool synced;
    int64_t sequenceNum;
};

struct OfflineState {
    std::string uavId;
    std::string state;
    std::string currentTaskId;
    std::string disconnectedAt;
    std::string reconnectedAt;
    nlohmann::json stateHistory;
    int64_t telemetrySequence;
    int64_t eventSequence;
};

struct TelemetryQuery {
    std::optional<std::string> uavId;
    std::optional<std::string> startTime;
    std::optional<std::string> endTime;
    std::optional<bool> synced;
    std::optional<int> limit;
    std::optional<int> offset;
};

class LocalStore {
public:
    explicit LocalStore(const std::string& dbPath);
    ~LocalStore();

    LocalStore(const LocalStore&) = delete;
    LocalStore& operator=(const LocalStore&) = delete;
    LocalStore(LocalStore&&) noexcept;
    LocalStore& operator=(LocalStore&&) noexcept;

    StoreResult initialize();
    void close();
    bool isOpen() const;
    StoreResult vacuum();
    StoreResult backup(const std::string& backupPath);

    StoreResult beginTransaction();
    StoreResult commitTransaction();
    StoreResult rollbackTransaction();
    bool inTransaction() const;

    StoreResult saveTask(const OfflineTask& task);
    StoreResult updateTaskStatus(const std::string& taskId, const std::string& status, 
                                  const std::optional<nlohmann::json>& logEntry = std::nullopt);
    StoreResult updateTaskProgress(const std::string& taskId, int progress);
    StoreResult incrementTaskRetry(const std::string& taskId);
    
    std::pair<std::vector<OfflineTask>, StoreResult> loadTasksByStatus(const std::string& status);
    std::pair<std::vector<OfflineTask>, StoreResult> loadTasksByUavId(const std::string& uavId);
    std::pair<std::optional<OfflineTask>, StoreResult> loadTask(const std::string& taskId);
    std::pair<std::vector<OfflineTask>, StoreResult> loadAllActiveTasks();
    
    StoreResult deleteTask(const std::string& taskId);
    StoreResult deleteTasksByStatus(const std::string& status);
    StoreResult archiveCompletedTasks(const std::string& archiveTableSuffix);
    
    StoreResult saveTelemetry(const TelemetryRecord& record);
    StoreResult saveTelemetryBatch(const std::vector<TelemetryRecord>& records);
    
    std::pair<std::vector<TelemetryRecord>, StoreResult> queryTelemetry(const TelemetryQuery& query);
    std::pair<std::vector<TelemetryRecord>, StoreResult> loadUnsyncedTelemetry(int limit = 100);
    std::pair<int, StoreResult> getUnsyncedTelemetryCount();
    std::pair<int64_t, StoreResult> getMaxTelemetrySequence();
    
    StoreResult markTelemetrySynced(const std::vector<int64_t>& ids);
    StoreResult markTelemetrySyncedBySequence(int64_t maxSequence);
    StoreResult deleteSyncedTelemetryBefore(const std::string& timestamp);
    StoreResult deleteSyncedTelemetryByCount(int keepCount);
    
    StoreResult saveEvent(const OfflineEvent& event);
    StoreResult saveEventBatch(const std::vector<OfflineEvent>& events);
    
    std::pair<std::vector<OfflineEvent>, StoreResult> loadUnsyncedEvents(int limit = 100);
    std::pair<int, StoreResult> getUnsyncedEventCount();
    std::pair<int64_t, StoreResult> getMaxEventSequence();
    
    StoreResult markEventsSynced(const std::vector<int64_t>& ids);
    StoreResult markEventsSyncedBySequence(int64_t maxSequence);
    
    StoreResult saveOfflineState(const OfflineState& state);
    std::pair<std::optional<OfflineState>, StoreResult> loadOfflineState(const std::string& uavId);
    StoreResult updateOfflineState(const std::string& uavId, const std::string& state,
                                    const std::optional<std::string>& taskId = std::nullopt);
    StoreResult appendStateHistory(const std::string& uavId, const nlohmann::json& historyEntry);
    
    std::pair<std::vector<nlohmann::json>, StoreResult> getStatistics();
    StoreResult clearAllData();
    StoreResult clearSyncedData();
    
    std::string getLastErrorMessage() const;

private:
    StoreResult createTables();
    StoreResult createIndexes();
    StoreResult migrateSchema();
    int getCurrentSchemaVersion();
    StoreResult setSchemaVersion(int version);
    
    StoreResult executeSql(const std::string& sql);
    StoreResult executeSqlWithCallback(const std::string& sql, 
                                        int (*callback)(void*, int, char**, char**),
                                        void* callbackArg);
    
    StoreResult prepareStatement(const std::string& sql, sqlite3_stmt** stmt);
    void finalizeStatement(sqlite3_stmt* stmt);
    
    TelemetryRecord extractTelemetryFromStatement(sqlite3_stmt* stmt);
    OfflineEvent extractEventFromStatement(sqlite3_stmt* stmt);
    OfflineTask extractTaskFromStatement(sqlite3_stmt* stmt);
    OfflineState extractStateFromStatement(sqlite3_stmt* stmt);
    
    std::string getCurrentTimestamp() const;
    int64_t getNextSequence(const std::string& uavId, const std::string& sequenceType);
    
    mutable std::mutex mutex_;
    sqlite3* db_;
    std::string dbPath_;
    bool initialized_;
    bool inTransaction_;
    std::string lastErrorMessage_;
    
    static constexpr int CURRENT_SCHEMA_VERSION = 1;
    static constexpr int DEFAULT_PAGE_SIZE = 4096;
    static constexpr int DEFAULT_CACHE_SIZE = -64000;
};

} // namespace nodeagent
