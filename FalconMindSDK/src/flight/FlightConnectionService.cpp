#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

namespace falconmind::sdk::flight {

FlightConnectionService::FlightConnectionService() = default;

FlightConnectionService::~FlightConnectionService() {
    disconnect();
}

bool FlightConnectionService::connect(const FlightConnectionConfig& cfg) {
    if (connected_) {
        return true;
    }

    sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        std::perror("socket");
        return false;
    }

    cfg_ = cfg;
    // 设置为非阻塞，用于 pollState() 非阻塞读取
    int flags = fcntl(sock_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
    }
    connected_ = true;
    std::cout << "[FlightConnectionService] UDP connect to "
              << cfg_.remoteAddress << ":" << cfg_.remotePort << std::endl;
    return true;
}

void FlightConnectionService::disconnect() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
    connected_ = false;
}

bool FlightConnectionService::sendCommand(const FlightCommand& cmd) {
    if (!connected_ || sock_ < 0) {
        std::cerr << "[FlightConnectionService] sendCommand: not connected" << std::endl;
        return false;
    }

    std::string payload;
    if (!encodeMavlinkCommand(cmd, payload)) {
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(cfg_.remotePort));
    ::inet_pton(AF_INET, cfg_.remoteAddress.c_str(), &addr.sin_addr);

    auto ret = ::sendto(sock_, payload.data(), payload.size(), 0,
                        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        std::perror("[FlightConnectionService] sendto");
        return false;
    }
    // 为避免在日志中出现二进制乱码，这里只输出简单摘要信息
    std::cout << "[FlightConnectionService] sendCommand: msgid=76, len="
              << payload.size() << " bytes" << std::endl;
    return true;
}

std::optional<FlightState> FlightConnectionService::pollState() {
    if (!connected_ || sock_ < 0) {
        return std::nullopt;
    }

    // 尝试一次非阻塞读取（若无数据直接返回 empty）
    std::uint8_t buf[256];
    ssize_t n = ::recv(sock_, buf, sizeof(buf), MSG_DONTWAIT);
    if (n <= 0) {
        return std::nullopt;
    }

    if (n < 12) { // 最小 MAVLink 帧长度检查
        return std::nullopt;
    }

    std::uint8_t stx = buf[0];
    if (stx != 0xFE && stx != 0xFD) {
        return std::nullopt;
    }

    std::uint8_t len = buf[1];
    std::uint8_t msgid = 0;
    const std::uint8_t* payload = nullptr;

    if (stx == 0xFE) {
        // MAVLink v1: STX LEN SEQ SYSID COMPID MSGID PAYLOAD CRC
        if (n < 6 + len + 2) return std::nullopt;
        msgid = buf[5];
        payload = &buf[6];
    } else {
        // MAVLink v2: STX LEN incompatFlags compatFlags SEQ SYSID COMPID MSGID[3] PAYLOAD CRC
        if (n < 10 + len + 2) return std::nullopt;
        msgid = buf[7]; // 我们当前只关心低 8 位
        payload = &buf[10];
    }

    // 解析 MAVLink v1 GLOBAL_POSITION_INT (msgid 33) 和 ATTITUDE (msgid 30)
    std::lock_guard<std::mutex> lk(stateMutex_);

    if (msgid == 33 && len >= 28) { // GLOBAL_POSITION_INT
        // payload 布局（小端）：
        // uint32 time_boot_ms
        // int32  lat
        // int32  lon
        // int32  alt
        // int32  relative_alt
        // int16  vx
        // int16  vy
        // int16  vz
        // uint16 hdg
        std::int32_t lat_i = 0, lon_i = 0, alt_i = 0;
        std::int16_t vx_i = 0, vy_i = 0, vz_i = 0;

        std::size_t off = 4; // 跳过 time_boot_ms
        std::memcpy(&lat_i, payload + off, 4); off += 4;
        std::memcpy(&lon_i, payload + off, 4); off += 4;
        std::memcpy(&alt_i, payload + off, 4); off += 4;
        off += 4; // relative_alt
        std::memcpy(&vx_i, payload + off, 2); off += 2;
        std::memcpy(&vy_i, payload + off, 2); off += 2;
        std::memcpy(&vz_i, payload + off, 2); off += 2;

        lastState_.lat = static_cast<double>(lat_i) / 1e7;
        lastState_.lon = static_cast<double>(lon_i) / 1e7;
        lastState_.alt = static_cast<double>(alt_i) / 1000.0; // mm → m
        lastState_.vx  = static_cast<double>(vx_i) / 100.0;   // cm/s → m/s
        lastState_.vy  = static_cast<double>(vy_i) / 100.0;
        lastState_.vz  = static_cast<double>(vz_i) / 100.0;

        return lastState_;
    } else if (msgid == 30 && len >= 28) { // ATTITUDE
        // payload 布局（小端）：
        // uint32 time_boot_ms
        // float roll, pitch, yaw, rollspeed, pitchspeed, yawspeed
        float roll = 0.f, pitch = 0.f, yaw = 0.f;
        std::size_t off = 4;
        std::memcpy(&roll,  payload + off, 4); off += 4;
        std::memcpy(&pitch, payload + off, 4); off += 4;
        std::memcpy(&yaw,   payload + off, 4); off += 4;

        lastState_.roll  = static_cast<double>(roll);
        lastState_.pitch = static_cast<double>(pitch);
        lastState_.yaw   = static_cast<double>(yaw);

        return lastState_;
    }

    return std::nullopt;
}

FlightState FlightConnectionService::getLastState() const {
    std::lock_guard<std::mutex> lk(stateMutex_);
    return lastState_;
}

// MAVLink v1/v2 COMMAND_LONG 编码（不依赖外部库的精简实现）
bool FlightConnectionService::encodeMavlinkCommand(const FlightCommand& cmd, std::string& out) {
    // 参考 MAVLink v1 帧格式：STX(0xFE) LEN SEQ SYSID COMPID MSGID PAYLOAD CRC
    // 以及 MAVLink v2 帧格式：STX(0xFD) LEN incompatFlags compatFlags SEQ SYSID COMPID MSGID[3] PAYLOAD CRC
    constexpr std::uint8_t LEN    = 33;   // COMMAND_LONG payload 长度
    constexpr std::uint8_t MSG_ID = 76;   // COMMAND_LONG
    constexpr std::uint8_t CRC_EXTRA = 152; // COMMAND_LONG 对应 extra CRC（来自 MAVLink 规范）

    // 默认将 SDK 视作地面站：SYSID=255, COMPID=190
    const std::uint8_t sysid  = 255;
    const std::uint8_t compid = 190;

    // COMMAND_LONG payload 布局（小端）：
    // float param1..7 (7 * 4) + uint16_t command + uint8_t target_system
    // + uint8_t target_component + uint8_t confirmation = 33 bytes
    std::uint8_t payload[LEN]{};

    float p1 = 0.f, p2 = 0.f, p3 = 0.f, p4 = 0.f, p5 = 0.f, p6 = 0.f, p7 = 0.f;
    std::uint16_t command = 0;

    switch (cmd.type) {
        case FlightCommandType::Arm:
            command = 400; // MAV_CMD_COMPONENT_ARM_DISARM
            p1 = 1.f;
            break;
        case FlightCommandType::Disarm:
            command = 400;
            p1 = 0.f;
            break;
        case FlightCommandType::Takeoff:
            command = 22;  // MAV_CMD_NAV_TAKEOFF
            p7 = static_cast<float>(cmd.targetAlt);
            break;
        case FlightCommandType::Land:
            command = 21;  // MAV_CMD_NAV_LAND
            break;
        case FlightCommandType::ReturnToLaunch:
            command = 20;  // MAV_CMD_NAV_RETURN_TO_LAUNCH
            break;
    }

    std::size_t offset = 0;
    auto putFloat = [&](float v) {
        std::uint8_t* p = reinterpret_cast<std::uint8_t*>(&v);
        payload[offset++] = p[0];
        payload[offset++] = p[1];
        payload[offset++] = p[2];
        payload[offset++] = p[3];
    };

    putFloat(p1); putFloat(p2); putFloat(p3); putFloat(p4);
    putFloat(p5); putFloat(p6); putFloat(p7);

    // command (uint16_t, little-endian)
    payload[offset++] = static_cast<std::uint8_t>(command & 0xFF);
    payload[offset++] = static_cast<std::uint8_t>((command >> 8) & 0xFF);

    // target_system & target_component & confirmation
    std::uint8_t target_system    = 1; // 通常为 PX4 的 SYSID
    std::uint8_t target_component = 1; // Autopilot
    std::uint8_t confirmation     = 0;
    payload[offset++] = target_system;
    payload[offset++] = target_component;
    payload[offset++] = confirmation;

    // 计算 CRC：payload + msgId + CRC_EXTRA
    std::uint16_t crc = mavlinkCrcCalculate(payload, LEN, CRC_EXTRA);

    if (cfg_.mavlinkVersion == MavlinkVersion::V1) {
        constexpr std::uint8_t STX_V1 = 0xFE;
        std::uint8_t frame[6 + LEN + 2];
        frame[0] = STX_V1;
        frame[1] = LEN;
        frame[2] = seq_++;
        frame[3] = sysid;
        frame[4] = compid;
        frame[5] = MSG_ID;
        std::memcpy(&frame[6], payload, LEN);
        frame[6 + LEN]     = static_cast<std::uint8_t>(crc & 0xFF);
        frame[6 + LEN + 1] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out.assign(reinterpret_cast<char*>(frame), sizeof(frame));
    } else {
        // MAVLink v2 帧
        constexpr std::uint8_t STX_V2        = 0xFD;
        constexpr std::uint8_t incompatFlags = 0x00;
        constexpr std::uint8_t compatFlags   = 0x00;

        std::uint8_t frame[10 + LEN + 2];
        frame[0] = STX_V2;
        frame[1] = LEN;
        frame[2] = incompatFlags;
        frame[3] = compatFlags;
        frame[4] = seq_++;
        frame[5] = sysid;
        frame[6] = compid;
        // msgid 3 字节小端
        frame[7] = MSG_ID & 0xFF;
        frame[8] = 0;
        frame[9] = 0;
        std::memcpy(&frame[10], payload, LEN);
        frame[10 + LEN]     = static_cast<std::uint8_t>(crc & 0xFF);
        frame[10 + LEN + 1] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
        out.assign(reinterpret_cast<char*>(frame), sizeof(frame));
    }

    return true;
}

std::uint16_t FlightConnectionService::mavlinkCrcAccumulate(std::uint8_t data, std::uint16_t crc) const {
    // 标准 MAVLink CRC-16/MCRF4XX 累加实现
    data ^= static_cast<std::uint8_t>(crc & 0xFFu);
    data ^= static_cast<std::uint8_t>(data << 4);

    return static_cast<std::uint16_t>(
        (crc >> 8) ^ (static_cast<std::uint16_t>(data) << 8)
        ^ (static_cast<std::uint16_t>(data) << 3)
        ^ (static_cast<std::uint16_t>(data) >> 4));
}

std::uint16_t FlightConnectionService::mavlinkCrcCalculate(const std::uint8_t* buf, std::size_t len, std::uint8_t crcExtra) const {
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < len; ++i) {
        crc = mavlinkCrcAccumulate(buf[i], crc);
    }
    crc = mavlinkCrcAccumulate(crcExtra, crc);
    return crc;
}

} // namespace falconmind::sdk::flight

