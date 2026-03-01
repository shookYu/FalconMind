#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <tuple>
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
    
    // 允许地址重用
    int reuse = 1;
    if (::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::perror("setsockopt SO_REUSEADDR");
    }
    
    // 绑定到本地端口，用于接收PX4发送的数据
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(static_cast<uint16_t>(cfg_.remotePort));
    
    if (::bind(sock_, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr)) < 0) {
        std::perror("bind");
        ::close(sock_);
        sock_ = -1;
        return false;
    }
    
    std::cout << "[FlightConnectionService] Bound to port " << cfg_.remotePort << std::endl;
    
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
    
    // Debug: print received msgid
    static int debug_counter = 0;
    if (++debug_counter % 10 == 0) {
        std::cout << "[FlightConnectionService] Received msgid=" << (int)msgid 
                  << " len=" << (int)len << " stx=0x" << std::hex << (int)stx << std::dec << std::endl;
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
        float rollspeed = 0.f, pitchspeed = 0.f, yawspeed = 0.f;
        std::size_t off = 4;
        std::memcpy(&roll,  payload + off, 4); off += 4;
        std::memcpy(&pitch, payload + off, 4); off += 4;
        std::memcpy(&yaw,   payload + off, 4); off += 4;
        std::memcpy(&rollspeed,  payload + off, 4); off += 4;
        std::memcpy(&pitchspeed, payload + off, 4); off += 4;
        std::memcpy(&yawspeed,   payload + off, 4);

        lastState_.roll  = static_cast<double>(roll);
        lastState_.pitch = static_cast<double>(pitch);
        lastState_.yaw   = static_cast<double>(yaw);
        // Gyro data from attitude rates
        lastState_.gx = static_cast<double>(rollspeed);
        lastState_.gy = static_cast<double>(pitchspeed);
        lastState_.gz = static_cast<double>(yawspeed);

        return lastState_;
    } else if (msgid == 105 && len >= 61) { // HIGHRES_IMU
        // payload 布局（小端）：
        // uint64 time_usec
        // float xacc, yacc, zacc (m/s^2)
        // float xgyro, ygyro, zgyro (rad/s)
        // float xmag, ymag, zmag
        // float abs_pressure
        // float diff_pressure
        // float pressure_alt
        // float temperature
        // uint16 fields_updated
        float xacc = 0.f, yacc = 0.f, zacc = 0.f;
        float xgyro = 0.f, ygyro = 0.f, zgyro = 0.f;
        std::size_t off = 8; // 跳过 time_usec
        std::memcpy(&xacc,  payload + off, 4); off += 4;
        std::memcpy(&yacc,  payload + off, 4); off += 4;
        std::memcpy(&zacc,  payload + off, 4); off += 4;
        std::memcpy(&xgyro, payload + off, 4); off += 4;
        std::memcpy(&ygyro, payload + off, 4); off += 4;
        std::memcpy(&zgyro, payload + off, 4);

        lastState_.ax = static_cast<double>(xacc);
        lastState_.ay = static_cast<double>(yacc);
        lastState_.az = static_cast<double>(zacc);
        lastState_.gx = static_cast<double>(xgyro);
        lastState_.gy = static_cast<double>(ygyro);
        lastState_.gz = static_cast<double>(zgyro);
        
        static int imuDebug = 0;
        if (++imuDebug % 100 == 0) {
            std::cout << "[FlightConnectionService] Parsed HIGHRES_IMU: ax=" << lastState_.ax 
                      << " ay=" << lastState_.ay << " az=" << lastState_.az << std::endl;
        }

        return lastState_;
    } else if (msgid == 24 && len >= 30) { // GPS_RAW_INT
        // payload 布局（小端）：
        // uint64 time_usec
        // uint8 fix_type
        // int32 lat
        // int32 lon
        // int32 alt (mm)
        // uint16 eph (hdop * 100)
        // uint16 epv
        // uint16 vel
        // uint16 cog
        // uint8 satellites_visible
        std::uint8_t fixType = 0;
        std::uint8_t numSats = 0;
        std::uint16_t eph = 9999;
        std::size_t off = 8; // 跳过 time_usec
        std::memcpy(&fixType, payload + off, 1); off += 1;
        off += 1; // 跳过 lat, lon, alt for now (use GLOBAL_POSITION_INT instead)
        std::memcpy(&eph, payload + off + 12, 2);
        off = 8 + 1 + 1 + 4 + 4 + 4 + 2 + 2 + 2 + 2; // 定位到 satellites_visible
        std::memcpy(&numSats, payload + off, 1);

        lastState_.gpsFixType = static_cast<int>(fixType);
        lastState_.numSat = static_cast<int>(numSats);
        lastState_.hdop = static_cast<double>(eph) / 100.0;

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

// ==================== 任务上传功能实现 ====================

bool FlightConnectionService::uploadMission(
    const std::vector<std::tuple<double, double, double>>& waypoints) {
    if (!connected_ || sock_ < 0) {
        std::cerr << "[FlightConnectionService] uploadMission: not connected" << std::endl;
        return false;
    }

    if (waypoints.empty()) {
        std::cerr << "[FlightConnectionService] uploadMission: empty waypoint list" << std::endl;
        return false;
    }

    const uint16_t totalCount = static_cast<uint16_t>(waypoints.size());
    std::cout << "[FlightConnectionService] 开始上传 " << totalCount << " 个航点..." << std::endl;

    // 步骤1: 清除现有任务
    if (!clearMission()) {
        std::cerr << "[FlightConnectionService] 清除现有任务失败" << std::endl;
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 步骤2: 发送MISSION_COUNT
    std::string countMsg;
    if (!encodeMissionCount(totalCount, countMsg)) {
        std::cerr << "[FlightConnectionService] 编码MISSION_COUNT失败" << std::endl;
        return false;
    }

    // 发送MISSION_COUNT并等待MISSION_REQUEST
    bool countSent = false;
    int retryCount = 0;
    const int maxRetries = 5;
    
    while (!countSent && retryCount < maxRetries) {
        // 发送COUNT
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(cfg_.remotePort));
        ::inet_pton(AF_INET, cfg_.remoteAddress.c_str(), &addr.sin_addr);
        
        auto ret = ::sendto(sock_, countMsg.data(), countMsg.size(), 0,
                            reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (ret < 0) {
            std::perror("[FlightConnectionService] sendto MISSION_COUNT");
            return false;
        }
        
        std::cout << "[FlightConnectionService] 发送MISSION_COUNT: " << totalCount << std::endl;
        
        // 等待MISSION_REQUEST或MISSION_ACK
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(
                  std::chrono::steady_clock::now() - start).count() < 2) {
            int msgId = receiveMessage(100);
            if (msgId == 40) { // MISSION_REQUEST
                countSent = true;
                break;
            } else if (msgId == 47) { // MISSION_ACK
                std::cout << "[FlightConnectionService] 收到MISSION_ACK, 任务已清除或完成" << std::endl;
                countSent = true;
                break;
            }
        }
        
        if (!countSent) {
            retryCount++;
            std::cout << "[FlightConnectionService] 未收到MISSION_REQUEST, 重试(" << retryCount << "/" << maxRetries << ")" << std::endl;
        }
    }

    if (!countSent) {
        std::cerr << "[FlightConnectionService] 发送MISSION_COUNT失败, 未收到响应" << std::endl;
        return false;
    }

    // 步骤3: 逐个发送MISSION_ITEM_INT
    uint16_t currentSeq = 0;
    while (currentSeq < totalCount) {
        // 发送当前航点
        const auto& wp = waypoints[currentSeq];
        double lat = std::get<0>(wp);
        double lon = std::get<1>(wp);
        double alt = std::get<2>(wp);
        
        std::string itemMsg;
        if (!encodeMissionItemInt(currentSeq, totalCount, lat, lon, static_cast<float>(alt), itemMsg)) {
            std::cerr << "[FlightConnectionService] 编码MISSION_ITEM_INT失败, seq=" << currentSeq << std::endl;
            return false;
        }
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(cfg_.remotePort));
        ::inet_pton(AF_INET, cfg_.remoteAddress.c_str(), &addr.sin_addr);
        
        auto ret = ::sendto(sock_, itemMsg.data(), itemMsg.size(), 0,
                            reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (ret < 0) {
            std::perror("[FlightConnectionService] sendto MISSION_ITEM_INT");
            return false;
        }
        
        if (currentSeq % 5 == 0 || currentSeq == totalCount - 1) {
            std::cout << "[FlightConnectionService] 发送航点 " << (currentSeq + 1) << "/" << totalCount << std::endl;
        }
        
        // 等待下一个MISSION_REQUEST或MISSION_ACK
        bool itemAcked = false;
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(
                  std::chrono::steady_clock::now() - start).count() < 3) {
            int msgId = receiveMessage(100);
            if (msgId == 40) { // MISSION_REQUEST
                // 解析请求的序列号
                // 这里简化处理,假设按顺序请求
                itemAcked = true;
                currentSeq++;
                break;
            } else if (msgId == 47) { // MISSION_ACK
                std::cout << "[FlightConnectionService] 收到MISSION_ACK, 任务上传完成" << std::endl;
                itemAcked = true;
                currentSeq = totalCount; // 结束循环
                break;
            }
        }
        
        if (!itemAcked) {
            // 超时,重试当前航点
            std::cout << "[FlightConnectionService] 航点 " << currentSeq << " 超时,重试..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[FlightConnectionService] 任务上传完成: " << totalCount << " 个航点" << std::endl;
    return true;
}

bool FlightConnectionService::clearMission() {
    if (!connected_ || sock_ < 0) {
        std::cerr << "[FlightConnectionService] clearMission: not connected" << std::endl;
        return false;
    }

    std::string clearMsg;
    if (!encodeMissionClearAll(clearMsg)) {
        std::cerr << "[FlightConnectionService] 编码MISSION_CLEAR_ALL失败" << std::endl;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(cfg_.remotePort));
    ::inet_pton(AF_INET, cfg_.remoteAddress.c_str(), &addr.sin_addr);

    auto ret = ::sendto(sock_, clearMsg.data(), clearMsg.size(), 0,
                        reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        std::perror("[FlightConnectionService] sendto MISSION_CLEAR_ALL");
        return false;
    }

    std::cout << "[FlightConnectionService] 发送MISSION_CLEAR_ALL" << std::endl;
    
    // 等待MISSION_ACK
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now() - start).count() < 2) {
        int msgId = receiveMessage(100);
        if (msgId == 47) { // MISSION_ACK
            std::cout << "[FlightConnectionService] 收到MISSION_ACK, 任务已清除" << std::endl;
            return true;
        }
    }

    // 即使没有收到ACK也返回true,因为命令已发送
    return true;
}

int FlightConnectionService::receiveMessage(int timeoutMs) {
    if (!connected_ || sock_ < 0) {
        return -1;
    }

    // 使用poll等待数据
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock_, &readfds);

    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int ready = ::select(sock_ + 1, &readfds, nullptr, nullptr, &tv);
    if (ready <= 0) {
        return -1; // 超时或无数据
    }

    std::lock_guard<std::mutex> lk(receiveMutex_);
    
    receiveBuffer_.resize(1024);
    ssize_t n = ::recv(sock_, receiveBuffer_.data(), receiveBuffer_.size(), 0);
    if (n <= 0) {
        return -1;
    }

    receiveBuffer_.resize(static_cast<size_t>(n));

    // 解析MAVLink帧
    if (n < 10) {
        return -1;
    }

    uint8_t stx = receiveBuffer_[0];
    if (stx != 0xFE && stx != 0xFD) {
        return -1;
    }

    uint8_t msgId = 0;
    if (stx == 0xFE) {
        // MAVLink v1
        msgId = receiveBuffer_[5];
    } else {
        // MAVLink v2
        msgId = receiveBuffer_[7];
    }

    lastReceivedMsgId_ = msgId;
    return static_cast<int>(msgId);
}

bool FlightConnectionService::encodeMissionItemInt(uint16_t seq, uint16_t total, 
    double lat, double lon, float alt, std::string& out) {
    // MISSION_ITEM_INT (msgid 73)
    constexpr uint8_t LEN = 37;
    constexpr uint8_t MSG_ID = 73;
    constexpr uint8_t CRC_EXTRA = 38;

    const uint8_t sysid = 255;   // GCS
    const uint8_t compid = 190;  // MISSION_PLANNER

    // Payload: MISSION_ITEM_INT
    // uint16_t target_system
    // uint16_t target_component
    // uint16_t seq
    // uint8_t frame
    // uint16_t command
    // uint8_t current
    // uint8_t autocontinue
    // float param1, param2, param3, param4
    // int32_t x (lat * 1e7)
    // int32_t y (lon * 1e7)
    // float z (alt)
    // uint8_t mission_type
    uint8_t payload[LEN] = {};
    size_t offset = 0;

    // target_system, target_component (uint16)
    payload[offset++] = 1; payload[offset++] = 0;  // target_system = 1
    payload[offset++] = 1; payload[offset++] = 0;  // target_component = 1

    // seq (uint16)
    payload[offset++] = static_cast<uint8_t>(seq & 0xFF);
    payload[offset++] = static_cast<uint8_t>((seq >> 8) & 0xFF);

    // frame (uint8) - MAV_FRAME_GLOBAL_RELATIVE_ALT = 3
    payload[offset++] = 3;

    // command (uint16) - MAV_CMD_NAV_WAYPOINT = 16
    payload[offset++] = 16; payload[offset++] = 0;

    // current (uint8) - 1 if this is the current waypoint
    payload[offset++] = (seq == 0) ? 1 : 0;

    // autocontinue (uint8)
    payload[offset++] = 1;

    // param1-4 (float) - hold time, acceptance radius, pass radius, yaw
    float holdTime = 0.0f, acceptRadius = 3.0f, passRadius = 0.0f, yaw = 0.0f;
    auto putFloat = [&](float v) {
        uint8_t* p = reinterpret_cast<uint8_t*>(&v);
        payload[offset++] = p[0];
        payload[offset++] = p[1];
        payload[offset++] = p[2];
        payload[offset++] = p[3];
    };
    putFloat(holdTime);
    putFloat(acceptRadius);
    putFloat(passRadius);
    putFloat(yaw);

    // x (int32) - latitude * 1e7
    int32_t latInt = static_cast<int32_t>(lat * 1e7);
    uint8_t* p = reinterpret_cast<uint8_t*>(&latInt);
    payload[offset++] = p[0]; payload[offset++] = p[1];
    payload[offset++] = p[2]; payload[offset++] = p[3];

    // y (int32) - longitude * 1e7
    int32_t lonInt = static_cast<int32_t>(lon * 1e7);
    p = reinterpret_cast<uint8_t*>(&lonInt);
    payload[offset++] = p[0]; payload[offset++] = p[1];
    payload[offset++] = p[2]; payload[offset++] = p[3];

    // z (float) - altitude
    putFloat(alt);

    // mission_type (uint8)
    payload[offset++] = 0; // MAV_MISSION_TYPE_MISSION

    // CRC
    uint16_t crc = mavlinkCrcCalculate(payload, LEN, CRC_EXTRA);

    // Build frame (MAVLink v1)
    constexpr uint8_t STX_V1 = 0xFE;
    uint8_t frame[6 + LEN + 2];
    frame[0] = STX_V1;
    frame[1] = LEN;
    frame[2] = seq_++;
    frame[3] = sysid;
    frame[4] = compid;
    frame[5] = MSG_ID;
    memcpy(&frame[6], payload, LEN);
    frame[6 + LEN] = static_cast<uint8_t>(crc & 0xFF);
    frame[6 + LEN + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    out.assign(reinterpret_cast<char*>(frame), sizeof(frame));
    return true;
}

bool FlightConnectionService::encodeMissionCount(uint16_t count, std::string& out) {
    // MISSION_COUNT (msgid 44)
    constexpr uint8_t LEN = 4;
    constexpr uint8_t MSG_ID = 44;
    constexpr uint8_t CRC_EXTRA = 189;

    const uint8_t sysid = 255;
    const uint8_t compid = 190;

    // Payload:
    // uint16_t target_system
    // uint16_t count
    uint8_t payload[LEN] = {};
    payload[0] = 1; payload[1] = 0;  // target_system = 1
    payload[2] = static_cast<uint8_t>(count & 0xFF);
    payload[3] = static_cast<uint8_t>((count >> 8) & 0xFF);

    uint16_t crc = mavlinkCrcCalculate(payload, LEN, CRC_EXTRA);

    constexpr uint8_t STX_V1 = 0xFE;
    uint8_t frame[6 + LEN + 2];
    frame[0] = STX_V1;
    frame[1] = LEN;
    frame[2] = seq_++;
    frame[3] = sysid;
    frame[4] = compid;
    frame[5] = MSG_ID;
    memcpy(&frame[6], payload, LEN);
    frame[6 + LEN] = static_cast<uint8_t>(crc & 0xFF);
    frame[6 + LEN + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    out.assign(reinterpret_cast<char*>(frame), sizeof(frame));
    return true;
}

bool FlightConnectionService::encodeMissionClearAll(std::string& out) {
    // MISSION_CLEAR_ALL (msgid 45)
    constexpr uint8_t LEN = 2;
    constexpr uint8_t MSG_ID = 45;
    constexpr uint8_t CRC_EXTRA = 232;

    const uint8_t sysid = 255;
    const uint8_t compid = 190;

    // Payload:
    // uint16_t target_system
    uint8_t payload[LEN] = {};
    payload[0] = 1; payload[1] = 0;  // target_system = 1

    uint16_t crc = mavlinkCrcCalculate(payload, LEN, CRC_EXTRA);

    constexpr uint8_t STX_V1 = 0xFE;
    uint8_t frame[6 + LEN + 2];
    frame[0] = STX_V1;
    frame[1] = LEN;
    frame[2] = seq_++;
    frame[3] = sysid;
    frame[4] = compid;
    frame[5] = MSG_ID;
    memcpy(&frame[6], payload, LEN);
    frame[6 + LEN] = static_cast<uint8_t>(crc & 0xFF);
    frame[6 + LEN + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    out.assign(reinterpret_cast<char*>(frame), sizeof(frame));
    return true;
}

} // namespace falconmind::sdk::flight

