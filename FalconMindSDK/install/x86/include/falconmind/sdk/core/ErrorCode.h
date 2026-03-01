/**
 * @file ErrorCode.h
 * @brief FalconMindSDK 错误码系统
 * 
 * 提供详细的错误分类和错误信息，替代简单的 bool 返回值
 * 
 * @example
 * @code
 * Result<Pipeline> result = createPipeline(config);
 * if (!result) {
 *     std::cerr << "Error: " << result.error().message() << std::endl;
 *     // 或者获取错误码
 *     if (result.error().code() == ErrorCode::NodeNotFound) {
 *         // 处理特定错误
 *     }
 * }
 * @endcode
 */

#pragma once

#include <cstring>
#include <string>
#include <optional>
#include <variant>

namespace falconmind {
namespace sdk {
namespace core {

/**
 * @brief 错误码枚举
 * 
 * 错误码格式: 0xCCSSSS
 * - CC: 组件类别 (Category)
 * - SSSS: 具体错误 (Specific)
 */
enum class ErrorCode : uint32_t {
    // 通用错误 (0x00xxxx)
    Success = 0x000000,
    Unknown = 0x000001,
    NotImplemented = 0x000002,
    InvalidArgument = 0x000003,
    OutOfMemory = 0x000004,
    Timeout = 0x000005,
    Cancelled = 0x000006,
    
    // Pipeline 错误 (0x01xxxx)
    PipelineInvalidState = 0x010001,
    PipelineAlreadyRunning = 0x010002,
    PipelineNotRunning = 0x010003,
    PipelineCreationFailed = 0x010004,
    
    // Node 错误 (0x02xxxx)
    NodeNotFound = 0x020001,
    NodeAlreadyExists = 0x020002,
    NodeCreationFailed = 0x020003,
    NodeConfigurationFailed = 0x020004,
    NodeStartFailed = 0x020005,
    NodeStopFailed = 0x020006,
    NodeInvalidType = 0x020007,
    
    // Pad/Connection 错误 (0x03xxxx)
    PadNotFound = 0x030001,
    PadAlreadyExists = 0x030002,
    PadTypeMismatch = 0x030003,
    PadAlreadyConnected = 0x030004,
    PadNotConnected = 0x030005,
    ConnectionFailed = 0x030006,
    InvalidLink = 0x030007,
    
    // Flow 错误 (0x04xxxx)
    FlowParseError = 0x040001,
    FlowInvalidJson = 0x040002,
    FlowMissingField = 0x040003,
    FlowNodeCreationFailed = 0x040004,
    FlowConnectionFailed = 0x040005,
    FlowNotLoaded = 0x040006,
    FlowAlreadyRunning = 0x040007,
    
    // Detection 错误 (0x10xxxx)
    DetectionModelNotFound = 0x100001,
    DetectionModelLoadFailed = 0x100002,
    DetectionInferenceFailed = 0x100003,
    DetectionInvalidInput = 0x100004,
    DetectionBackendNotAvailable = 0x100005,
    
    // Tracking 错误 (0x11xxxx)
    TrackingNotInitialized = 0x110001,
    TrackingLost = 0x110002,
    TrackingInvalidInput = 0x110003,
    
    // Flight 错误 (0x20xxxx)
    FlightConnectionFailed = 0x200001,
    FlightCommandFailed = 0x200002,
    FlightTimeout = 0x200003,
    GeofenceViolation = 0x200004,
    
    // Sensor 错误 (0x30xxxx)
    SensorNotAvailable = 0x300001,
    SensorReadFailed = 0x300002,
    SensorCalibrationFailed = 0x300003,
    
    // Mission 错误 (0x40xxxx)
    MissionInvalidParameters = 0x400001,
    MissionExecutionFailed = 0x400002,
    MissionAborted = 0x400003,
};

/**
 * @brief 获取错误码对应的字符串名称
 */
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::Unknown: return "Unknown error";
        case ErrorCode::NotImplemented: return "Not implemented";
        case ErrorCode::InvalidArgument: return "Invalid argument";
        case ErrorCode::OutOfMemory: return "Out of memory";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::Cancelled: return "Cancelled";
        
        case ErrorCode::PipelineInvalidState: return "Pipeline invalid state";
        case ErrorCode::PipelineAlreadyRunning: return "Pipeline already running";
        case ErrorCode::PipelineNotRunning: return "Pipeline not running";
        case ErrorCode::PipelineCreationFailed: return "Pipeline creation failed";
        
        case ErrorCode::NodeNotFound: return "Node not found";
        case ErrorCode::NodeAlreadyExists: return "Node already exists";
        case ErrorCode::NodeCreationFailed: return "Node creation failed";
        case ErrorCode::NodeConfigurationFailed: return "Node configuration failed";
        case ErrorCode::NodeStartFailed: return "Node start failed";
        case ErrorCode::NodeStopFailed: return "Node stop failed";
        case ErrorCode::NodeInvalidType: return "Invalid node type";
        
        case ErrorCode::PadNotFound: return "Pad not found";
        case ErrorCode::PadAlreadyExists: return "Pad already exists";
        case ErrorCode::PadTypeMismatch: return "Pad type mismatch";
        case ErrorCode::PadAlreadyConnected: return "Pad already connected";
        case ErrorCode::PadNotConnected: return "Pad not connected";
        case ErrorCode::ConnectionFailed: return "Connection failed";
        case ErrorCode::InvalidLink: return "Invalid link";
        
        case ErrorCode::FlowParseError: return "Flow parse error";
        case ErrorCode::FlowInvalidJson: return "Flow invalid JSON";
        case ErrorCode::FlowMissingField: return "Flow missing required field";
        case ErrorCode::FlowNodeCreationFailed: return "Flow node creation failed";
        case ErrorCode::FlowConnectionFailed: return "Flow connection failed";
        case ErrorCode::FlowNotLoaded: return "Flow not loaded";
        case ErrorCode::FlowAlreadyRunning: return "Flow already running";
        
        case ErrorCode::DetectionModelNotFound: return "Detection model not found";
        case ErrorCode::DetectionModelLoadFailed: return "Detection model load failed";
        case ErrorCode::DetectionInferenceFailed: return "Detection inference failed";
        case ErrorCode::DetectionInvalidInput: return "Detection invalid input";
        case ErrorCode::DetectionBackendNotAvailable: return "Detection backend not available";
        
        case ErrorCode::TrackingNotInitialized: return "Tracking not initialized";
        case ErrorCode::TrackingLost: return "Tracking lost";
        case ErrorCode::TrackingInvalidInput: return "Tracking invalid input";
        
        case ErrorCode::FlightConnectionFailed: return "Flight connection failed";
        case ErrorCode::FlightCommandFailed: return "Flight command failed";
        case ErrorCode::FlightTimeout: return "Flight timeout";
        case ErrorCode::GeofenceViolation: return "Geofence violation";
        
        case ErrorCode::SensorNotAvailable: return "Sensor not available";
        case ErrorCode::SensorReadFailed: return "Sensor read failed";
        case ErrorCode::SensorCalibrationFailed: return "Sensor calibration failed";
        
        case ErrorCode::MissionInvalidParameters: return "Mission invalid parameters";
        case ErrorCode::MissionExecutionFailed: return "Mission execution failed";
        case ErrorCode::MissionAborted: return "Mission aborted";
        
        default: return "Unknown error code";
    }
}

/**
 * @brief 错误信息类
 */
class Error {
public:
    Error() : code_(ErrorCode::Success) {}
    
    explicit Error(ErrorCode code) 
        : code_(code), message_(errorCodeToString(code)) {}
    
    Error(ErrorCode code, std::string message)
        : code_(code), message_(std::move(message)) {}
    
    Error(ErrorCode code, std::string message, std::string context)
        : code_(code), message_(std::move(message)), context_(std::move(context)) {}
    
    // Getters
    ErrorCode code() const { return code_; }
    const std::string& message() const { return message_; }
    const std::string& context() const { return context_; }
    
    // Boolean conversion
    explicit operator bool() const { return code_ != ErrorCode::Success; }
    bool isSuccess() const { return code_ == ErrorCode::Success; }
    bool isError() const { return code_ != ErrorCode::Success; }
    
    // Category check
    bool isPipelineError() const {
        return (static_cast<uint32_t>(code_) & 0xFF0000) == 0x010000;
    }
    bool isNodeError() const {
        return (static_cast<uint32_t>(code_) & 0xFF0000) == 0x020000;
    }
    bool isFlowError() const {
        return (static_cast<uint32_t>(code_) & 0xFF0000) == 0x040000;
    }
    
    // Format full error message
    std::string toString() const {
        std::string result = message_;
        if (!context_.empty()) {
            result += " [Context: " + context_ + "]";
        }
        return result;
    }
    
private:
    ErrorCode code_;
    std::string message_;
    std::string context_;
};

/**
 * @brief Result 类型 - 包含值或错误
 * 
 * @tparam T 成功时返回的类型
 */
template<typename T>
class Result {
public:
    using ValueType = T;
    
    // Success constructors
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    
    // Error constructor
    Result(const Error& error) : data_(error) {}
    Result(Error&& error) : data_(std::move(error)) {}
    
    // ErrorCode constructor
    Result(ErrorCode code) : data_(Error(code)) {}
    
    // Check status
    bool hasValue() const { return std::holds_alternative<T>(data_); }
    bool hasError() const { return std::holds_alternative<Error>(data_); }
    explicit operator bool() const { return hasValue(); }
    
    // Access value (throws/crashes if error - use with caution)
    T& value() { return std::get<T>(data_); }
    const T& value() const { return std::get<T>(data_); }
    
    // Access error
    Error& error() { return std::get<Error>(data_); }
    const Error& error() const { return std::get<Error>(data_); }
    
    // Safe value access
    std::optional<T> valueOr(std::nullopt_t) const {
        if (hasValue()) return std::get<T>(data_);
        return std::nullopt;
    }
    
    T valueOr(const T& defaultValue) const {
        if (hasValue()) return std::get<T>(data_);
        return defaultValue;
    }
    
    // Arrow operator for value access
    T* operator->() { return &std::get<T>(data_); }
    const T* operator->() const { return &std::get<T>(data_); }
    
    // Dereference operator
    T& operator*() { return std::get<T>(data_); }
    const T& operator*() const { return std::get<T>(data_); }
    
private:
    std::variant<T, Error> data_;
};

/**
 * @brief void 特化的 Result 类型（仅用于错误返回）
 */
template<>
class Result<void> {
public:
    Result() : error_(ErrorCode::Success) {}  // Success
    Result(const Error& error) : error_(error) {}
    Result(Error&& error) : error_(std::move(error)) {}
    Result(ErrorCode code) : error_(Error(code)) {}
    
    explicit operator bool() const { return error_.isSuccess(); }
    bool isSuccess() const { return error_.isSuccess(); }
    bool isError() const { return error_.isError(); }
    
    const Error& error() const { return error_; }
    
private:
    Error error_;
};

// Helper macros for error handling
#define RETURN_IF_ERROR(result) \
    do { \
        auto&& _res = (result); \
        if (!_res) return _res.error(); \
    } while(0)

#define RETURN_ERROR(code) return Error(code)
#define RETURN_ERROR_WITH_MSG(code, msg) return Error(code, msg)

} // namespace core
} // namespace sdk
} // namespace falconmind
