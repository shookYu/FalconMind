/**
 * @file ErrorCode.h
 * @brief Unified error code system for FalconMind SDK
 * 
 * Provides standardized error codes across all SDK modules.
 * Replace bool returns with rich error information.
 */

#pragma once

#include <string>
#include <system_error>

namespace falconmind {
namespace sdk {

/**
 * @brief Comprehensive error codes for all SDK operations
 * 
 * Error code ranges:
 * - 0: Success
 * - 1000-1999: Configuration errors
 * - 2000-2999: Device/hardware errors  
 * - 3000-3999: Model/AI inference errors
 * - 4000-4999: Pipeline/runtime errors
 * - 5000-5999: Network/communication errors
 * - 6000-6999: System/resource errors
 */
enum class ErrorCode : int {
    // Success
    Success = 0,
    
    // Configuration errors (1000-1999)
    InvalidConfig = 1001,
    MissingRequiredParameter = 1002,
    InvalidParameterValue = 1003,
    ConfigFileNotFound = 1004,
    ConfigParseError = 1005,
    IncompatibleVersion = 1006,
    
    // Device errors (2000-2999)
    DeviceNotFound = 2001,
    DeviceOpenFailed = 2002,
    DeviceIOError = 2003,
    DeviceDisconnected = 2004,
    DeviceBusy = 2005,
    DeviceNotSupported = 2006,
    CameraNotAvailable = 2101,
    ImuNotAvailable = 2102,
    GnssNotAvailable = 2103,
    LidarNotAvailable = 2104,
    
    // Model/AI errors (3000-3999)
    ModelLoadFailed = 3001,
    ModelNotSupported = 3002,
    ModelVersionMismatch = 3003,
    ModelCorrupted = 3004,
    InferenceFailed = 3005,
    BackendNotAvailable = 3006,
    NpuInitializationFailed = 3101,
    NpuInferenceTimeout = 3102,
    NpuOutOfMemory = 3103,
    
    // Pipeline errors (4000-4999)
    PipelineNotRunning = 4001,
    PipelineAlreadyRunning = 4002,
    NodeNotFound = 4003,
    PadNotConnected = 4004,
    InvalidStateTransition = 4005,
    PipelineConfigurationError = 4006,
    DataTypeMismatch = 4007,
    BufferOverflow = 4008,
    
    // Communication errors (5000-5999)
    ConnectionFailed = 5001,
    ConnectionLost = 5002,
    Timeout = 5003,
    ProtocolError = 5004,
    MavlinkInitFailed = 5101,
    MavlinkSendFailed = 5102,
    MavlinkRecvFailed = 5103,
    MavlinkTimeout = 5104,
    
    // System errors (6000-6999)
    OutOfMemory = 6001,
    PermissionDenied = 6002,
    NotImplemented = 6003,
    InternalError = 6004,
    ThreadCreationFailed = 6005,
    MutexLockFailed = 6006,
    ResourceExhausted = 6007,
};

/**
 * @brief Convert error code to human-readable string
 */
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        
        case ErrorCode::InvalidConfig: return "Invalid configuration";
        case ErrorCode::MissingRequiredParameter: return "Missing required parameter";
        case ErrorCode::InvalidParameterValue: return "Invalid parameter value";
        case ErrorCode::ConfigFileNotFound: return "Configuration file not found";
        case ErrorCode::ConfigParseError: return "Configuration parse error";
        case ErrorCode::IncompatibleVersion: return "Incompatible version";
        
        case ErrorCode::DeviceNotFound: return "Device not found";
        case ErrorCode::DeviceOpenFailed: return "Failed to open device";
        case ErrorCode::DeviceIOError: return "Device I/O error";
        case ErrorCode::DeviceDisconnected: return "Device disconnected";
        case ErrorCode::DeviceBusy: return "Device busy";
        case ErrorCode::DeviceNotSupported: return "Device not supported";
        case ErrorCode::CameraNotAvailable: return "Camera not available";
        case ErrorCode::ImuNotAvailable: return "IMU not available";
        case ErrorCode::GnssNotAvailable: return "GNSS not available";
        case ErrorCode::LidarNotAvailable: return "LiDAR not available";
        
        case ErrorCode::ModelLoadFailed: return "Failed to load model";
        case ErrorCode::ModelNotSupported: return "Model format not supported";
        case ErrorCode::ModelVersionMismatch: return "Model version mismatch";
        case ErrorCode::ModelCorrupted: return "Model file corrupted";
        case ErrorCode::InferenceFailed: return "Inference failed";
        case ErrorCode::BackendNotAvailable: return "Inference backend not available";
        case ErrorCode::NpuInitializationFailed: return "NPU initialization failed";
        case ErrorCode::NpuInferenceTimeout: return "NPU inference timeout";
        case ErrorCode::NpuOutOfMemory: return "NPU out of memory";
        
        case ErrorCode::PipelineNotRunning: return "Pipeline not running";
        case ErrorCode::PipelineAlreadyRunning: return "Pipeline already running";
        case ErrorCode::NodeNotFound: return "Node not found";
        case ErrorCode::PadNotConnected: return "Pad not connected";
        case ErrorCode::InvalidStateTransition: return "Invalid state transition";
        case ErrorCode::PipelineConfigurationError: return "Pipeline configuration error";
        case ErrorCode::DataTypeMismatch: return "Data type mismatch";
        case ErrorCode::BufferOverflow: return "Buffer overflow";
        
        case ErrorCode::ConnectionFailed: return "Connection failed";
        case ErrorCode::ConnectionLost: return "Connection lost";
        case ErrorCode::Timeout: return "Operation timeout";
        case ErrorCode::ProtocolError: return "Protocol error";
        case ErrorCode::MavlinkInitFailed: return "MAVLink initialization failed";
        case ErrorCode::MavlinkSendFailed: return "MAVLink send failed";
        case ErrorCode::MavlinkRecvFailed: return "MAVLink receive failed";
        case ErrorCode::MavlinkTimeout: return "MAVLink timeout";
        
        case ErrorCode::OutOfMemory: return "Out of memory";
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::NotImplemented: return "Feature not implemented";
        case ErrorCode::InternalError: return "Internal error";
        case ErrorCode::ThreadCreationFailed: return "Thread creation failed";
        case ErrorCode::MutexLockFailed: return "Mutex lock failed";
        case ErrorCode::ResourceExhausted: return "Resource exhausted";
        
        default: return "Unknown error";
    }
}

/**
 * @brief Error category for std::error_code integration
 */
class FalconMindErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "falconmind";
    }
    
    std::string message(int ev) const override {
        return errorCodeToString(static_cast<ErrorCode>(ev));
    }
};

inline const std::error_category& getFalconMindErrorCategory() {
    static FalconMindErrorCategory category;
    return category;
}

inline std::error_code make_error_code(ErrorCode e) {
    return std::error_code(static_cast<int>(e), getFalconMindErrorCategory());
}

} // namespace sdk
} // namespace falconmind

// Enable std::error_code integration
namespace std {
template<>
struct is_error_code_enum<falconmind::sdk::ErrorCode> : true_type {};
}
