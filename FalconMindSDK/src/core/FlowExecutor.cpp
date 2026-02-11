#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include <fstream>
#include <sstream>
#include <iostream>

// 使用cpp-httplib进行HTTP请求
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#endif

namespace falconmind::sdk::core {

FlowExecutor::FlowExecutor()
    : pipeline_(nullptr)
    , running_(false) {
    // 阶段4优化：NodeFactory现在使用线程安全的延迟初始化
    // createNode会自动初始化，这里不需要手动初始化
    // 但为了兼容性，保留检查逻辑（getRegisteredTypes也会触发初始化）
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
}

FlowExecutor::~FlowExecutor() {
    stop();
}

bool FlowExecutor::loadFlow(const std::string& flow_json) {
    try {
        // 只解析一次JSON，避免重复解析
        flow_definition_json_ = json::parse(flow_json);
        return parseFlowDefinition(flow_definition_json_);
    } catch (const json::exception& e) {
        std::cerr << "FlowExecutor: Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }
}

bool FlowExecutor::loadFlowFromFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "FlowExecutor: Failed to open file: " << file_path << std::endl;
        return false;
    }
    
    try {
        // 直接解析JSON对象，避免转换为字符串再解析
        file >> flow_definition_json_;
        file.close();
        return parseFlowDefinition(flow_definition_json_);
    } catch (const json::exception& e) {
        std::cerr << "FlowExecutor: Failed to parse JSON from file: " << e.what() << std::endl;
        return false;
    }
}

bool FlowExecutor::loadFlowFromBuilder(const std::string& builder_url,
                                      const std::string& project_id,
                                      const std::string& flow_id) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    try {
        // 解析URL
        std::string host;
        std::string path;
        int port = 80;
        bool use_https = false;
        
        // 简单的URL解析
        std::string url = builder_url;
        if (url.find("https://") == 0) {
            use_https = true;
            port = 443;
            url = url.substr(8);  // 移除 "https://"
        } else if (url.find("http://") == 0) {
            url = url.substr(7);  // 移除 "http://"
        }
        
        // 查找端口号
        size_t colon_pos = url.find(':');
        size_t slash_pos = url.find('/');
        
        if (colon_pos != std::string::npos && (slash_pos == std::string::npos || colon_pos < slash_pos)) {
            host = url.substr(0, colon_pos);
            port = std::stoi(url.substr(colon_pos + 1, slash_pos - colon_pos - 1));
            if (slash_pos != std::string::npos) {
                path = url.substr(slash_pos);
            }
        } else if (slash_pos != std::string::npos) {
            host = url.substr(0, slash_pos);
            path = url.substr(slash_pos);
        } else {
            host = url;
        }
        
        // 构建请求路径
        std::string request_path = "/projects/" + project_id + "/flows/" + flow_id + "/export";
        if (!path.empty() && path != "/") {
            request_path = path + request_path;
        }
        
        // 创建HTTP客户端
        httplib::Client cli(host.c_str(), port);
        cli.set_connection_timeout(5, 0);  // 5秒超时
        cli.set_read_timeout(5, 0);
        
        // 发送GET请求
        auto res = cli.Get(request_path.c_str());
        
        if (!res) {
            std::cerr << "FlowExecutor: HTTP request failed (no response)" << std::endl;
            return false;
        }
        
        if (res->status != 200) {
            std::cerr << "FlowExecutor: HTTP request failed with status: " << res->status << std::endl;
            return false;
        }
        
        // 解析响应JSON并加载Flow
        return loadFlow(res->body);
        
    } catch (const std::exception& e) {
        std::cerr << "FlowExecutor: Error loading flow from Builder: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "FlowExecutor: loadFlowFromBuilder requires cpp-httplib library" << std::endl;
    std::cerr << "  Builder URL: " << builder_url << std::endl;
    std::cerr << "  Project ID: " << project_id << std::endl;
    std::cerr << "  Flow ID: " << flow_id << std::endl;
    std::cerr << "  Please enable CPPHTTPLIB_OPENSSL_SUPPORT in CMakeLists.txt" << std::endl;
    return false;
#endif
}

bool FlowExecutor::parseFlowDefinition(const json& flow_json_obj) {
    try {
        // 使用已解析的JSON对象，避免重复解析
        // 使用const引用减少拷贝
        const json& j = flow_json_obj;
        
        // 提取基本信息
        if (!j.contains("flow_id")) {
            std::cerr << "FlowExecutor: Flow definition missing flow_id" << std::endl;
            return false;
        }
        
        flow_id_ = j["flow_id"].get<std::string>();
        flow_name_ = j.value("name", "");
        flow_version_ = j.value("version", "1.0");
        
        // 解析节点定义
        if (!j.contains("nodes") || !j["nodes"].is_array()) {
            std::cerr << "FlowExecutor: Flow definition missing or invalid nodes array" << std::endl;
            return false;
        }
        
        // 预分配空间以减少重新分配
        node_definitions_.clear();
        node_definitions_.reserve(j["nodes"].size());
        
        for (const auto& node_json : j["nodes"]) {
            NodeDefinition node_def;
            node_def.node_id = node_json["node_id"].get<std::string>();
            node_def.template_id = node_json["template_id"].get<std::string>();
            
            // 提取parameters（如果存在）- 使用引用避免拷贝
            if (node_json.contains("parameters")) {
                node_def.parameters_json = node_json["parameters"];  // nlohmann::json支持移动语义
            } else {
                node_def.parameters_json = json::object();
            }
            
            node_definitions_.push_back(std::move(node_def));
        }
        
        // 解析边定义
        if (!j.contains("edges") || !j["edges"].is_array()) {
            std::cerr << "FlowExecutor: Flow definition missing or invalid edges array" << std::endl;
            return false;
        }
        
        // 预分配空间以减少重新分配
        edge_definitions_.clear();
        edge_definitions_.reserve(j["edges"].size());
        
        for (const auto& edge_json : j["edges"]) {
            EdgeDefinition edge_def;
            edge_def.edge_id = edge_json.value("edge_id", "");
            edge_def.from_node_id = edge_json["from_node_id"].get<std::string>();
            edge_def.from_port = edge_json["from_port"].get<std::string>();
            edge_def.to_node_id = edge_json["to_node_id"].get<std::string>();
            edge_def.to_port = edge_json["to_port"].get<std::string>();
            
            edge_definitions_.push_back(std::move(edge_def));
        }
        
        std::cout << "FlowExecutor: Parsed flow definition" << std::endl;
        std::cout << "  Flow ID: " << flow_id_ << std::endl;
        std::cout << "  Flow Name: " << flow_name_ << std::endl;
        std::cout << "  Flow Version: " << flow_version_ << std::endl;
        std::cout << "  Nodes: " << node_definitions_.size() << std::endl;
        std::cout << "  Edges: " << edge_definitions_.size() << std::endl;
        
        return true;
    } catch (const json::exception& e) {
        std::cerr << "FlowExecutor: JSON parsing error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "FlowExecutor: Error parsing flow definition: " << e.what() << std::endl;
        return false;
    }
}

bool FlowExecutor::createNodes() {
    if (node_definitions_.empty()) {
        std::cerr << "FlowExecutor: No node definitions to create" << std::endl;
        return false;
    }
    
    nodes_.clear();
    
    for (const auto& node_def : node_definitions_) {
        // 创建Node实例
        auto node = NodeFactory::createNode(node_def.template_id, node_def.node_id, nullptr);
        if (!node) {
            std::cerr << "FlowExecutor: Failed to create node: " << node_def.node_id 
                      << " (template: " << node_def.template_id << ")" << std::endl;
            return false;
        }
        
        // 配置节点参数
        if (!configureNodeParams(node, node_def.template_id, node_def.parameters_json)) {
            std::cerr << "FlowExecutor: Failed to configure node parameters: " << node_def.node_id << std::endl;
            // 继续执行，参数配置失败不影响节点创建
        }
        
        nodes_[node_def.node_id] = node;
        std::cout << "FlowExecutor: Created and configured node: " << node_def.node_id 
                  << " (template: " << node_def.template_id << ")" << std::endl;
    }
    
    return true;
}

// 参数验证辅助函数
namespace {
    // 验证地理坐标点
    bool validateGeoPoint(const json& point_json, std::string& error_msg) {
        if (!point_json.is_object()) {
            error_msg = "GeoPoint must be an object";
            return false;
        }
        
        if (!point_json.contains("lat") || !point_json["lat"].is_number()) {
            error_msg = "GeoPoint missing or invalid 'lat' field";
            return false;
        }
        
        if (!point_json.contains("lon") || !point_json["lon"].is_number()) {
            error_msg = "GeoPoint missing or invalid 'lon' field";
            return false;
        }
        
        double lat = point_json["lat"].get<double>();
        double lon = point_json["lon"].get<double>();
        
        // 值范围验证
        if (lat < -90.0 || lat > 90.0) {
            error_msg = "Latitude must be between -90.0 and 90.0, got: " + std::to_string(lat);
            return false;
        }
        
        if (lon < -180.0 || lon > 180.0) {
            error_msg = "Longitude must be between -180.0 and 180.0, got: " + std::to_string(lon);
            return false;
        }
        
        // alt是可选的，但如果存在必须是数字
        if (point_json.contains("alt") && !point_json["alt"].is_number()) {
            error_msg = "GeoPoint 'alt' field must be a number";
            return false;
        }
        
        return true;
    }
    
    // 验证搜索区域参数
    bool validateSearchArea(const json& area_json, std::string& error_msg) {
        if (!area_json.is_object()) {
            error_msg = "search_area must be an object";
            return false;
        }
        
        // 验证polygon字段
        if (area_json.contains("polygon")) {
            if (!area_json["polygon"].is_array()) {
                error_msg = "search_area.polygon must be an array";
                return false;
            }
            
            const auto& polygon = area_json["polygon"];
            if (polygon.size() < 3) {
                error_msg = "search_area.polygon must have at least 3 points, got: " + std::to_string(polygon.size());
                return false;
            }
            
            // 验证每个点
            for (size_t i = 0; i < polygon.size(); ++i) {
                std::string point_error;
                if (!validateGeoPoint(polygon[i], point_error)) {
                    error_msg = "search_area.polygon[" + std::to_string(i) + "]: " + point_error;
                    return false;
                }
            }
        }
        
        // 验证高度范围
        if (area_json.contains("min_altitude")) {
            if (!area_json["min_altitude"].is_number()) {
                error_msg = "search_area.min_altitude must be a number";
                return false;
            }
            double min_alt = area_json["min_altitude"].get<double>();
            if (min_alt < 0.0 || min_alt > 10000.0) {
                error_msg = "search_area.min_altitude must be between 0.0 and 10000.0, got: " + std::to_string(min_alt);
                return false;
            }
        }
        
        if (area_json.contains("max_altitude")) {
            if (!area_json["max_altitude"].is_number()) {
                error_msg = "search_area.max_altitude must be a number";
                return false;
            }
            double max_alt = area_json["max_altitude"].get<double>();
            if (max_alt < 0.0 || max_alt > 10000.0) {
                error_msg = "search_area.max_altitude must be between 0.0 and 10000.0, got: " + std::to_string(max_alt);
                return false;
            }
        }
        
        // 验证min_altitude < max_altitude
        if (area_json.contains("min_altitude") && area_json.contains("max_altitude")) {
            double min_alt = area_json["min_altitude"].get<double>();
            double max_alt = area_json["max_altitude"].get<double>();
            if (min_alt >= max_alt) {
                error_msg = "search_area.min_altitude must be less than max_altitude";
                return false;
            }
        }
        
        return true;
    }
    
    // 验证搜索参数
    bool validateSearchParams(const json& params_json, std::string& error_msg) {
        if (!params_json.is_object()) {
            error_msg = "search_params must be an object";
            return false;
        }
        
        // 验证搜索模式
        if (params_json.contains("pattern")) {
            if (!params_json["pattern"].is_string()) {
                error_msg = "search_params.pattern must be a string";
                return false;
            }
            std::string pattern = params_json["pattern"].get<std::string>();
            if (pattern != "LAWN_MOWER" && pattern != "SPIRAL" && pattern != "ZIGZAG" && 
                pattern != "SECTOR" && pattern != "WAYPOINT_LIST") {
                error_msg = "search_params.pattern must be one of: LAWN_MOWER, SPIRAL, ZIGZAG, SECTOR, WAYPOINT_LIST";
                return false;
            }
        }
        
        // 验证高度
        if (params_json.contains("altitude")) {
            if (!params_json["altitude"].is_number()) {
                error_msg = "search_params.altitude must be a number";
                return false;
            }
            double altitude = params_json["altitude"].get<double>();
            if (altitude < 0.0 || altitude > 10000.0) {
                error_msg = "search_params.altitude must be between 0.0 and 10000.0, got: " + std::to_string(altitude);
                return false;
            }
        }
        
        // 验证速度
        if (params_json.contains("speed")) {
            if (!params_json["speed"].is_number()) {
                error_msg = "search_params.speed must be a number";
                return false;
            }
            double speed = params_json["speed"].get<double>();
            if (speed < 0.0 || speed > 50.0) {
                error_msg = "search_params.speed must be between 0.0 and 50.0 m/s, got: " + std::to_string(speed);
                return false;
            }
        }
        
        // 验证间距
        if (params_json.contains("spacing")) {
            if (!params_json["spacing"].is_number()) {
                error_msg = "search_params.spacing must be a number";
                return false;
            }
            double spacing = params_json["spacing"].get<double>();
            if (spacing < 1.0 || spacing > 1000.0) {
                error_msg = "search_params.spacing must be between 1.0 and 1000.0 meters, got: " + std::to_string(spacing);
                return false;
            }
        }
        
        // 验证悬停时间
        if (params_json.contains("loiter_time")) {
            if (!params_json["loiter_time"].is_number()) {
                error_msg = "search_params.loiter_time must be a number";
                return false;
            }
            double loiter_time = params_json["loiter_time"].get<double>();
            if (loiter_time < 0.0 || loiter_time > 3600.0) {
                error_msg = "search_params.loiter_time must be between 0.0 and 3600.0 seconds, got: " + std::to_string(loiter_time);
                return false;
            }
        }
        
        // 验证enable_detection（如果存在必须是布尔值）
        if (params_json.contains("enable_detection")) {
            if (!params_json["enable_detection"].is_boolean()) {
                error_msg = "search_params.enable_detection must be a boolean";
                return false;
            }
        }
        
        return true;
    }
}

bool FlowExecutor::configureNodeParams(std::shared_ptr<Node> node,
                                      const std::string& template_id,
                                      const json& params_json) {
    if (params_json.is_null() || params_json.empty()) {
        return true;  // 无参数需要配置
    }
    
    try {
        // 根据模板ID配置不同类型的节点
        if (template_id == "search_path_planner") {
            auto planner = std::dynamic_pointer_cast<mission::SearchPathPlannerNode>(node);
            if (!planner) {
                std::cerr << "FlowExecutor: Node is not SearchPathPlannerNode" << std::endl;
                return false;
            }
            
            // 参数格式和值范围验证
            std::string validation_error;
            
            // 验证搜索区域
            if (params_json.contains("search_area")) {
                if (!validateSearchArea(params_json["search_area"], validation_error)) {
                    std::cerr << "FlowExecutor: Invalid search_area: " << validation_error << std::endl;
                    return false;
                }
            }
            
            // 验证搜索参数
            if (params_json.contains("search_params")) {
                if (!validateSearchParams(params_json["search_params"], validation_error)) {
                    std::cerr << "FlowExecutor: Invalid search_params: " << validation_error << std::endl;
                    return false;
                }
            }
            
            // 解析搜索区域（验证通过后）
            // 使用引用避免重复访问JSON对象
            if (params_json.contains("search_area")) {
                mission::SearchArea area;
                const auto& area_json = params_json["search_area"];  // 使用const引用
                
                // 解析多边形 - 预分配空间
                if (area_json.contains("polygon") && area_json["polygon"].is_array()) {
                    const auto& polygon_array = area_json["polygon"];
                    area.polygon.reserve(polygon_array.size());
                    for (const auto& point_json : polygon_array) {
                        mission::GeoPoint point;
                        point.lat = point_json["lat"].get<double>();
                        point.lon = point_json["lon"].get<double>();
                        point.alt = point_json.value("alt", 0.0);
                        area.polygon.push_back(point);
                    }
                }
                
                area.minAltitude = area_json.value("min_altitude", 0.0);
                area.maxAltitude = area_json.value("max_altitude", 100.0);
                
                planner->setSearchArea(area);
            }
            
            // 解析搜索参数（验证通过后）
            // 使用引用避免重复访问JSON对象
            if (params_json.contains("search_params")) {
                mission::SearchParams params;
                const auto& params_json_obj = params_json["search_params"];  // 使用const引用
                
                // 解析搜索模式 - 一次性获取并转换
                std::string pattern_str = params_json_obj.value("pattern", "LAWN_MOWER");
                if (pattern_str == "LAWN_MOWER") {
                    params.pattern = mission::SearchPattern::LAWN_MOWER;
                } else if (pattern_str == "SPIRAL") {
                    params.pattern = mission::SearchPattern::SPIRAL;
                } else if (pattern_str == "ZIGZAG") {
                    params.pattern = mission::SearchPattern::ZIGZAG;
                } else if (pattern_str == "SECTOR") {
                    params.pattern = mission::SearchPattern::SECTOR;
                } else if (pattern_str == "WAYPOINT_LIST") {
                    params.pattern = mission::SearchPattern::WAYPOINT_LIST;
                } else {
                    params.pattern = mission::SearchPattern::LAWN_MOWER;
                }
                
                // 一次性获取所有参数值，减少JSON访问次数
                params.altitude = params_json_obj.value("altitude", 50.0);
                params.speed = params_json_obj.value("speed", 10.0);
                params.spacing = params_json_obj.value("spacing", 20.0);
                params.loiterTime = params_json_obj.value("loiter_time", 2.0);
                params.enableDetection = params_json_obj.value("enable_detection", false);
                
                planner->setSearchParams(params);
            }
        }
        // 其他节点类型的参数配置可以在这里添加
        // else if (template_id == "event_reporter") { ... }
        // else if (template_id == "camera_source") { ... }
        
        return true;
    } catch (const json::exception& e) {
        std::cerr << "FlowExecutor: Error configuring node parameters: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "FlowExecutor: Error configuring node: " << e.what() << std::endl;
        return false;
    }
}

bool FlowExecutor::connectNodes() {
    if (!pipeline_) {
        std::cerr << "FlowExecutor: Pipeline not created" << std::endl;
        return false;
    }
    
    // 如果没有边定义，直接返回成功（允许没有连接的Flow）
    if (edge_definitions_.empty()) {
        return true;
    }
    
    for (const auto& edge_def : edge_definitions_) {
        auto from_it = nodes_.find(edge_def.from_node_id);
        auto to_it = nodes_.find(edge_def.to_node_id);
        
        if (from_it == nodes_.end() || to_it == nodes_.end()) {
            std::cerr << "FlowExecutor: Node not found for edge: " << edge_def.edge_id 
                      << " (from: " << edge_def.from_node_id 
                      << ", to: " << edge_def.to_node_id << ")" << std::endl;
            return false;
        }
        
        // 验证Pad是否存在
        auto from_node = from_it->second;
        auto to_node = to_it->second;
        auto from_pad = from_node->getPad(edge_def.from_port);
        auto to_pad = to_node->getPad(edge_def.to_port);
        
        if (!from_pad) {
            std::cerr << "FlowExecutor: Source pad not found: " << edge_def.from_node_id 
                      << ":" << edge_def.from_port << std::endl;
            return false;
        }
        
        if (!to_pad) {
            std::cerr << "FlowExecutor: Sink pad not found: " << edge_def.to_node_id 
                      << ":" << edge_def.to_port << std::endl;
            return false;
        }
        
        bool success = pipeline_->link(edge_def.from_node_id, edge_def.from_port,
                                      edge_def.to_node_id, edge_def.to_port);
        if (!success) {
            std::cerr << "FlowExecutor: Failed to connect nodes: " 
                      << edge_def.from_node_id << ":" << edge_def.from_port 
                      << " -> " << edge_def.to_node_id << ":" << edge_def.to_port << std::endl;
            std::cerr << "  Source pad type: " << static_cast<int>(from_pad->type()) << std::endl;
            std::cerr << "  Sink pad type: " << static_cast<int>(to_pad->type()) << std::endl;
            return false;
        }
        
        std::cout << "FlowExecutor: Connected " << edge_def.from_node_id 
                  << ":" << edge_def.from_port << " -> " 
                  << edge_def.to_node_id << ":" << edge_def.to_port << std::endl;
    }
    
    return true;
}

bool FlowExecutor::start() {
    if (running_) {
        std::cerr << "FlowExecutor: Flow is already running" << std::endl;
        return false;
    }
    
    // 创建Pipeline
    PipelineConfig config;
    config.pipelineId = flow_id_;
    config.name = flow_name_;
    config.description = "Flow: " + flow_name_;
    
    pipeline_ = std::make_shared<Pipeline>(config);
    
    // 创建节点
    if (!createNodes()) {
        return false;
    }
    
    // 添加节点到Pipeline
    for (const auto& pair : nodes_) {
        if (!pipeline_->addNode(pair.second)) {
            std::cerr << "FlowExecutor: Failed to add node to pipeline: " << pair.first << std::endl;
            return false;
        }
    }
    
    // 连接节点
    if (!connectNodes()) {
        return false;
    }
    
    // 启动Pipeline
    if (!pipeline_->setState(PipelineState::Playing)) {
        std::cerr << "FlowExecutor: Failed to start pipeline" << std::endl;
        return false;
    }
    
    running_ = true;
    std::cout << "FlowExecutor: Flow started successfully" << std::endl;
    return true;
}

void FlowExecutor::stop() {
    if (!running_) {
        return;
    }
    
    if (pipeline_) {
        pipeline_->setState(PipelineState::Null);
    }
    
    running_ = false;
    std::cout << "FlowExecutor: Flow stopped" << std::endl;
}

bool FlowExecutor::isRunning() const {
    return running_;
}

bool FlowExecutor::updateFlow(const std::string& flow_json) {
    // 停止当前Flow
    stop();
    
    // 清理
    pipeline_.reset();
    nodes_.clear();
    node_definitions_.clear();
    edge_definitions_.clear();
    
    // 加载新Flow
    if (!loadFlow(flow_json)) {
        return false;
    }
    
    // 重新启动
    return start();
}

} // namespace falconmind::sdk::core
