// NodeAgent - Error code definitions
#pragma once

#include <cstdint>

namespace nodeagent {

// 错误码定义
enum class ErrorCode : int32_t {
    // 通用错误 (0x0000-0x0FFF)
    Success = 0,
    UnknownError = 0x0001,
    InvalidParameter = 0x0002,
    NotInitialized = 0x0003,
    AlreadyInitialized = 0x0004,
    OperationNotSupported = 0x0005,

    // 网络连接错误 (0x1000-0x1FFF)
    NetworkError = 0x1000,
    ConnectionFailed = 0x1001,
    ConnectionTimeout = 0x1002,
    ConnectionLost = 0x1003,
    SocketError = 0x1004,
    InvalidAddress = 0x1005,
    SendFailed = 0x1006,
    ReceiveFailed = 0x1007,
    DisconnectFailed = 0x1008,

    // MQTT 错误 (0x2000-0x2FFF)
    MqttNotEnabled = 0x2000,
    MqttConnectionFailed = 0x2001,
    MqttPublishFailed = 0x2002,
    MqttSubscribeFailed = 0x2003,
    MqttUnsubscribeFailed = 0x2004,
    MqttDisconnectFailed = 0x2005,

    // 消息处理错误 (0x3000-0x3FFF)
    MessageParseError = 0x3000,
    MessageSerializeError = 0x3001,
    InvalidMessageFormat = 0x3002,
    MessageTimeout = 0x3003,
    MessageAckFailed = 0x3004,

    // 命令/任务处理错误 (0x4000-0x4FFF)
    CommandParseError = 0x4000,
    CommandExecuteError = 0x4001,
    MissionParseError = 0x4002,
    MissionExecuteError = 0x4003,
    FlightServiceNotSet = 0x4004,
    FlightServiceNotConnected = 0x4005,

    // 多 UAV 错误 (0x5000-0x5FFF)
    UavAlreadyExists = 0x5000,
    UavNotFound = 0x5001,
    UavStartFailed = 0x5002,
    UavStopFailed = 0x5003,
};

// 错误码转换为字符串
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::UnknownError: return "UnknownError";
        case ErrorCode::InvalidParameter: return "InvalidParameter";
        case ErrorCode::NotInitialized: return "NotInitialized";
        case ErrorCode::AlreadyInitialized: return "AlreadyInitialized";
        case ErrorCode::OperationNotSupported: return "OperationNotSupported";
        case ErrorCode::NetworkError: return "NetworkError";
        case ErrorCode::ConnectionFailed: return "ConnectionFailed";
        case ErrorCode::ConnectionTimeout: return "ConnectionTimeout";
        case ErrorCode::ConnectionLost: return "ConnectionLost";
        case ErrorCode::SocketError: return "SocketError";
        case ErrorCode::InvalidAddress: return "InvalidAddress";
        case ErrorCode::SendFailed: return "SendFailed";
        case ErrorCode::ReceiveFailed: return "ReceiveFailed";
        case ErrorCode::DisconnectFailed: return "DisconnectFailed";
        case ErrorCode::MqttNotEnabled: return "MqttNotEnabled";
        case ErrorCode::MqttConnectionFailed: return "MqttConnectionFailed";
        case ErrorCode::MqttPublishFailed: return "MqttPublishFailed";
        case ErrorCode::MqttSubscribeFailed: return "MqttSubscribeFailed";
        case ErrorCode::MqttUnsubscribeFailed: return "MqttUnsubscribeFailed";
        case ErrorCode::MqttDisconnectFailed: return "MqttDisconnectFailed";
        case ErrorCode::MessageParseError: return "MessageParseError";
        case ErrorCode::MessageSerializeError: return "MessageSerializeError";
        case ErrorCode::InvalidMessageFormat: return "InvalidMessageFormat";
        case ErrorCode::MessageTimeout: return "MessageTimeout";
        case ErrorCode::MessageAckFailed: return "MessageAckFailed";
        case ErrorCode::CommandParseError: return "CommandParseError";
        case ErrorCode::CommandExecuteError: return "CommandExecuteError";
        case ErrorCode::MissionParseError: return "MissionParseError";
        case ErrorCode::MissionExecuteError: return "MissionExecuteError";
        case ErrorCode::FlightServiceNotSet: return "FlightServiceNotSet";
        case ErrorCode::FlightServiceNotConnected: return "FlightServiceNotConnected";
        case ErrorCode::UavAlreadyExists: return "UavAlreadyExists";
        case ErrorCode::UavNotFound: return "UavNotFound";
        case ErrorCode::UavStartFailed: return "UavStartFailed";
        case ErrorCode::UavStopFailed: return "UavStopFailed";
        default: return "UnknownErrorCode";
    }
}

} // namespace nodeagent
