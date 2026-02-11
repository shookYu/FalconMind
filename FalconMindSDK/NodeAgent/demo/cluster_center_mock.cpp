// Cluster Center Mock - 简单的 TCP 服务器，接收 NodeAgent 上报的 Telemetry
// 支持 ACK 响应机制
// 支持将 Telemetry 转发到 Viewer 后端

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>

#ifndef NO_CURL
#include <curl/curl.h>
#endif

// 辅助函数：从下行消息中提取 requestId 并发送 ACK
void sendAckIfNeeded(int clientFd, const std::string& message) {
    // 提取消息类型和 payload
    std::string payload;
    if (message.find("CMD:") == 0) {
        payload = message.substr(4);
    } else if (message.find("MISSION:") == 0) {
        payload = message.substr(8);
    } else {
        return;  // 不是下行消息
    }

    try {
        // 解析 JSON payload，提取 requestId
        auto json = nlohmann::json::parse(payload);
        
        std::string requestId;
        if (json.contains("requestId") && json["requestId"].is_string()) {
            requestId = json["requestId"].get<std::string>();
        } else {
            // 如果没有 requestId，不发送 ACK
            return;
        }

        // 发送 ACK 响应：ACK:{requestId}\n
        std::string ackMessage = "ACK:" + requestId + "\n";
        ssize_t sent = send(clientFd, ackMessage.c_str(), ackMessage.length(), 0);
        if (sent > 0) {
            std::cout << "[cluster_center_mock] Sent ACK: " << requestId << std::endl;
        } else {
            std::cerr << "[cluster_center_mock] Failed to send ACK" << std::endl;
        }
    } catch (const nlohmann::json::parse_error& e) {
        // JSON 解析失败，不发送 ACK
        // std::cerr << "[cluster_center_mock] Failed to parse JSON for ACK: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // 其他错误，不发送 ACK
    }
}

#ifndef NO_CURL
// HTTP POST 回调函数（用于 libcurl）
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    (void)contents;
    (void)userp;
    return size * nmemb;
}

// 转发 Telemetry 到 Viewer 后端
void forwardTelemetryToViewer(const std::string& jsonMessage, const std::string& viewerUrl) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[cluster_center_mock] Failed to initialize CURL" << std::endl;
        return;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, viewerUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonMessage.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);  // 2 秒超时

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        // 静默失败，避免日志过多（Viewer 可能未启动）
        // std::cerr << "[cluster_center_mock] Failed to forward to Viewer: " 
        //           << curl_easy_strerror(res) << std::endl;
    } else {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            // std::cout << "[cluster_center_mock] Successfully forwarded telemetry to Viewer" << std::endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
#else
// 如果没有 libcurl，提供一个空实现
void forwardTelemetryToViewer(const std::string& jsonMessage, const std::string& viewerUrl) {
    (void)jsonMessage;
    (void)viewerUrl;
    // libcurl 未找到，转发功能被禁用
}
#endif

int main(int argc, char* argv[]) {
    int port = 8888;
    std::string viewerUrl = "http://127.0.0.1:9000/ingress/telemetry";
    bool forwardToViewer = true;

    if (argc >= 2) {
        port = std::stoi(argv[1]);
    }
    if (argc >= 3) {
        viewerUrl = argv[2];
    }
    if (argc >= 4) {
        forwardToViewer = (std::string(argv[3]) == "true" || std::string(argv[3]) == "1");
    }

#ifndef NO_CURL
    // 初始化 libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif

    std::cout << "[cluster_center_mock] Starting Cluster Center mock server on port " << port << std::endl;
    if (forwardToViewer) {
        std::cout << "[cluster_center_mock] Telemetry forwarding to Viewer: " << viewerUrl << std::endl;
    } else {
        std::cout << "[cluster_center_mock] Telemetry forwarding disabled" << std::endl;
    }

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "[cluster_center_mock] Failed to create socket" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[cluster_center_mock] Failed to bind to port " << port << std::endl;
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 5) < 0) {
        std::cerr << "[cluster_center_mock] Failed to listen" << std::endl;
        close(serverFd);
        return 1;
    }

    std::cout << "[cluster_center_mock] Listening for NodeAgent connections..." << std::endl;

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
        std::cerr << "[cluster_center_mock] Failed to accept connection" << std::endl;
        close(serverFd);
        return 1;
    }

    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
    std::cout << "[cluster_center_mock] Accepted connection from " << clientIp << std::endl;
    std::cout << "[cluster_center_mock] Type 'send CMD:<json>' or 'send MISSION:<json>' to send downlink message" << std::endl;
    std::cout << "[cluster_center_mock] ACK responses will be sent automatically for messages with 'requestId'" << std::endl;
    std::cout << "[cluster_center_mock] Type 'noack' to disable ACK, 'ack' to enable (default: enabled)" << std::endl;

    // 使用 select 实现双向通信（接收 Telemetry + 发送命令）
    fd_set readFds;
    std::string messageBuffer;
    char buffer[4096];
    int telemetryCount = 0;
    bool ackEnabled = true;  // ACK 响应开关

    while (true) {
        FD_ZERO(&readFds);
        FD_SET(clientFd, &readFds);
        FD_SET(STDIN_FILENO, &readFds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int maxFd = std::max(clientFd, STDIN_FILENO) + 1;
        int result = select(maxFd, &readFds, nullptr, nullptr, &timeout);

        if (result < 0) {
            break;
        }
        if (result == 0) {
            continue;  // 超时
        }

        // 处理来自 NodeAgent 的上行消息（Telemetry）
        if (FD_ISSET(clientFd, &readFds)) {
            ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    std::cout << "[cluster_center_mock] Client disconnected" << std::endl;
                } else {
                    std::cerr << "[cluster_center_mock] Error receiving data" << std::endl;
                }
                break;
            }

            // 优化：使用二进制方式接收，避免 '\0' 截断问题
            messageBuffer.append(buffer, n);

            // 处理完整的消息（以换行符分隔）
            // 优化：由于 JSON 现在是单行格式，可以简单地通过换行符识别完整消息
            size_t pos;
            while ((pos = messageBuffer.find('\n')) != std::string::npos) {
                std::string message = messageBuffer.substr(0, pos);
                messageBuffer.erase(0, pos + 1);

                // 跳过空消息和下行消息（CMD:/MISSION:）
                if (message.empty() || message.find("CMD:") == 0 || message.find("MISSION:") == 0) {
                    continue;
                }

                // 优化：按完整 JSON 消息打印（一次性打印整个 JSON）
                // 可选：格式化 JSON 以便阅读（当前是单行压缩格式）
                telemetryCount++;
                std::cout << "\n[Cluster Center] Received Telemetry #" << telemetryCount << " from NodeAgent:\n"
                          << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
                          << message << "\n"
                          << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;

                // 转发 Telemetry 到 Viewer 后端（异步，避免阻塞）
                if (forwardToViewer) {
                    // 在后台线程中转发，避免阻塞主循环
                    std::thread([message, viewerUrl]() {
                        forwardTelemetryToViewer(message, viewerUrl);
                    }).detach();
                }
            }
        }

        // 处理来自标准输入的命令（发送下行消息）
        if (FD_ISSET(STDIN_FILENO, &readFds)) {
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) {
                continue;
            }

            if (input.find("send ") == 0) {
                std::string cmd = input.substr(5);  // 跳过 "send "
                std::string message = cmd + "\n";
                ssize_t sent = send(clientFd, message.c_str(), message.length(), 0);
                if (sent > 0) {
                    std::cout << "[cluster_center_mock] Sent downlink message: " << cmd << std::endl;
                    
                    // 如果启用了 ACK，尝试从消息中提取 requestId 并发送 ACK
                    if (ackEnabled) {
                        sendAckIfNeeded(clientFd, cmd);
                    }
                } else {
                    std::cerr << "[cluster_center_mock] Failed to send message" << std::endl;
                }
            } else if (input == "ack") {
                ackEnabled = true;
                std::cout << "[cluster_center_mock] ACK responses enabled" << std::endl;
            } else if (input == "noack") {
                ackEnabled = false;
                std::cout << "[cluster_center_mock] ACK responses disabled" << std::endl;
            } else if (input == "quit" || input == "exit") {
                break;
            } else {
                std::cout << "[cluster_center_mock] Unknown command. Use 'send CMD:<json>' or 'send MISSION:<json>'" << std::endl;
                std::cout << "[cluster_center_mock] Commands: 'ack', 'noack', 'quit', 'exit'" << std::endl;
            }
        }
    }

    close(clientFd);
    close(serverFd);
#ifndef NO_CURL
    curl_global_cleanup();
#endif
    std::cout << "[cluster_center_mock] Shutdown" << std::endl;
    return 0;
}
