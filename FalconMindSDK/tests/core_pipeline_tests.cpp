#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Caps.h"
#include "falconmind/sdk/core/Bus.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"
#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "falconmind/sdk/perception/PerceptionPluginManager.h"
#include "falconmind/sdk/perception/OnnxRuntimeDetectorBackend.h"
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
#include "falconmind/sdk/perception/TensorRtDetectorBackend.h"
#include "falconmind/sdk/perception/DetectorConfigLoader.h"
#include "falconmind/sdk/perception/SimpleTrackerBackend.h"
#include "falconmind/sdk/perception/TrackingTransformNode.h"
#include "falconmind/sdk/perception/EnvironmentDetectionNode.h"
#include "falconmind/sdk/perception/LowLightAdaptationNode.h"
#include "falconmind/sdk/perception/VisualSlamNode.h"
#include "falconmind/sdk/perception/PoseTypes.h"
#include "falconmind/sdk/perception/ISlamServiceClient.h"
#include "falconmind/sdk/sensors/GnssSourceNode.h"
#include "falconmind/sdk/sensors/ImuSourceNode.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>
#include <vector>

using namespace falconmind::sdk::core;

namespace {

class DummyNode : public Node {
public:
    DummyNode(const std::string& id) : Node(id) {
        addPad(std::make_shared<Pad>("in", PadType::Sink));
        addPad(std::make_shared<Pad>("out", PadType::Source));
    }
};

void test_pipeline_add_and_link() {
    PipelineConfig cfg;
    cfg.pipelineId = "test_pipeline";

    Pipeline p(cfg);
    auto n1 = std::make_shared<DummyNode>("n1");
    auto n2 = std::make_shared<DummyNode>("n2");

    assert(p.addNode(n1));
    assert(!p.addNode(n1)); // duplicate should fail
    assert(p.addNode(n2));

    assert(p.link("n1", "out", "n2", "in"));    // both exist
    assert(!p.link("nX", "out", "n2", "in"));   // src missing
    assert(!p.link("n1", "out", "nY", "in"));   // dst missing

    assert(p.setState(PipelineState::Ready));
    assert(p.setState(PipelineState::Playing));
}

void test_node_and_pad_basic() {
    DummyNode node("nodeA");
    auto inPad = node.getPad("in");
    auto outPad = node.getPad("out");
    assert(inPad);
    assert(outPad);
    assert(inPad->type() == PadType::Sink);
    assert(outPad->type() == PadType::Source);
}

void test_pad_push_to_connections() {
    auto srcPad = std::make_shared<Pad>("out", PadType::Source);
    auto sinkPad = std::make_shared<Pad>("in", PadType::Sink);
    std::vector<char> received;
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const char* p = static_cast<const char*>(data);
        received.assign(p, p + size);
    });
    assert(srcPad->connectTo(sinkPad, "sink_node", "in"));
    const char payload[] = "hello";
    srcPad->pushToConnections(payload, 5);
    assert(received.size() == 5);
    assert(std::memcmp(received.data(), "hello", 5) == 0);
}

void test_camera_frame_packet() {
    using namespace falconmind::sdk::sensors;
    CameraFramePacket h;
    h.width = 640;
    h.height = 480;
    h.stride = 640 * 3;
    std::strncpy(h.format, "RGB8", sizeof(h.format) - 1);
    h.format[sizeof(h.format) - 1] = '\0';
    assert(cameraFramePacketTotalSize(h, 3) == sizeof(CameraFramePacket) + static_cast<size_t>(h.stride) * static_cast<size_t>(h.height));
    assert(cameraFramePacketTotalSize(h, 3) == sizeof(CameraFramePacket) + 640u * 480u * 3u);
    CameraFramePacket empty{};
    assert(cameraFramePacketTotalSize(empty, 3) == sizeof(CameraFramePacket));
    std::vector<std::uint8_t> buf(sizeof(CameraFramePacket) + 10);
    auto* hp = reinterpret_cast<CameraFramePacket*>(buf.data());
    hp->width = 2;
    hp->height = 2;
    hp->stride = 6;
    const std::uint8_t* dataPtr = cameraFramePacketData(hp);
    assert(dataPtr == buf.data() + sizeof(CameraFramePacket));
}

void test_caps_properties() {
    Caps caps;
    caps.set("format", "image/rgb8");
    caps.set("width", "1920");
    caps.set("height", "1080");

    const auto& props = caps.properties();
    assert(props.at("format") == "image/rgb8");
    assert(props.at("width") == "1920");
    assert(props.at("height") == "1080");
}

void test_bus_publish_subscribe() {
    Bus bus;
    int count = 0;
    auto id = bus.subscribe([&](const BusMessage& msg) {
        ++count;
        assert(msg.category == "test");
        assert(msg.text == "hello");
    });

    BusMessage msg{"test", "hello"};
    bus.post(msg);
    assert(count == 1);

    bus.unsubscribe(id);
    bus.post(msg);
    assert(count == 1); // no further increments
}

void test_flight_connection_service_basic() {
    using namespace falconmind::sdk::flight;

    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;

    // 即使没有真实 PX4-SITL，在本地创建 UDP socket 也应该是安全的
    bool ok = svc.connect(cfg);
    assert(ok);
    assert(svc.isConnected());

    FlightCommand cmd;
    cmd.type = FlightCommandType::Arm;
    svc.sendCommand(cmd); // 不检查结果，仅验证不崩溃

    svc.disconnect();
    assert(!svc.isConnected());
}

void test_flight_nodes_basic() {
    using namespace falconmind::sdk::flight;

    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;
    bool ok = svc.connect(cfg);
    assert(ok);

    FlightStateSourceNode stateNode(svc);
    FlightCommandSinkNode cmdNode(svc);

    assert(stateNode.start());
    assert(cmdNode.start());

    // 设置一个待发送命令，并调用 process，验证不会崩溃
    FlightCommand cmd;
    cmd.type = FlightCommandType::Arm;
    cmdNode.setPendingCommand(cmd);
    cmdNode.process();

    // 调用一次状态轮询节点（当前 pollState 返回空 optional，只验证调用路径）
    stateNode.process();

    svc.disconnect();
}

void test_camera_source_node_basic() {
    using namespace falconmind::sdk::sensors;

    VideoSourceConfig cfg;
    cfg.sensorId = "cam0";
    cfg.device   = "/dev/video0";
    cfg.width    = 640;
    cfg.height   = 480;
    cfg.fps      = 30.0;

    CameraSourceNode camNode(cfg);
    std::unordered_map<std::string, std::string> params{
        {"device", "/dev/video0"},
        {"uri",    ""}
    };
    assert(camNode.configure(params));
    assert(camNode.start());
    camNode.process(); // 仅打印一条日志，验证调用路径
}

void test_camera_to_detection_pipeline() {
    using namespace falconmind::sdk::sensors;
    using namespace falconmind::sdk::perception;

    // 构造一个最简单的 camera_source → detection_transform Pipeline
    PipelineConfig cfg;
    cfg.pipelineId = "cam_det_pipeline";
    Pipeline p(cfg);

    VideoSourceConfig vcfg;
    vcfg.sensorId = "cam0";
    CameraSourceNode camNode(vcfg);
    DummyDetectionNode detNode;

    // 这里直接调用 Node 接口，不通过 NodeRegistry（后续可扩展）
    auto camPtr = std::make_shared<CameraSourceNode>(vcfg);
    auto detPtr = std::make_shared<DummyDetectionNode>();

    assert(p.addNode(camPtr));
    assert(p.addNode(detPtr));
    assert(p.link(camPtr->id(), "video_out", detPtr->id(), "video_in"));

    // 启动并执行一次 process，验证调用路径
    std::unordered_map<std::string, std::string> camParams{
        {"device", "/dev/video0"}
    };
    camPtr->configure(camParams);
    assert(camPtr->start());
    detPtr->configure({{"modelName", "dummy-yolo"}});
    assert(detPtr->start());

    camPtr->process();
    detPtr->process();
}

void test_perception_plugin_manager_with_onnx_backend() {
    using namespace falconmind::sdk::perception;

    PerceptionPluginManager mgr;

    // 注册 ONNXRuntime backend 工厂
    mgr.registerDetectorBackend("onnxruntime",
                                DetectionBackendType::OnnxRuntime,
                                []() { return std::make_shared<OnnxRuntimeDetectorBackend>(); });

    // 注册一个 YOLO 检测器描述（占位模型路径）
    DetectorDescriptor desc;
    desc.detectorId = "yolo_dummy_onnx";
    desc.name = "YOLO Dummy (ONNX)";
    desc.modelPath = "/path/to/yolo_dummy.onnx";
    desc.backendType = DetectionBackendType::OnnxRuntime;
    desc.inputWidth = 640;
    desc.inputHeight = 480;
    desc.numClasses = 80;
    mgr.registerDetectorDescriptor(desc);

    // 创建并调用一次 run()
    auto backend = mgr.createDetector("yolo_dummy_onnx");
    assert(backend);
    assert(backend->isLoaded());

    ImageView img{};
    img.width = 0;
    img.height = 0;
    img.stride = 0;
    img.pixelFormat = "RGB8";

    DetectionResult result;
    bool ok = backend->run(img, result);
    assert(ok);
}

void test_perception_plugin_manager_with_rknn_and_tensorrt() {
    using namespace falconmind::sdk::perception;

    PerceptionPluginManager mgr;

    mgr.registerDetectorBackend("rknn",
                                DetectionBackendType::Rknn,
                                []() { return std::make_shared<RknnDetectorBackend>(); });
    mgr.registerDetectorBackend("tensorrt",
                                DetectionBackendType::TensorRt,
                                []() { return std::make_shared<TensorRtDetectorBackend>(); });

    DetectorDescriptor descRknn;
    descRknn.detectorId = "yolo_rknn";
    descRknn.name = "YOLO RKNN";
    descRknn.modelPath = "/path/to/yolo_rknn.rknn";
    descRknn.backendType = DetectionBackendType::Rknn;
    mgr.registerDetectorDescriptor(descRknn);

    DetectorDescriptor descTrt;
    descTrt.detectorId = "yolo_trt";
    descTrt.name = "YOLO TensorRT";
    descTrt.modelPath = "/path/to/yolo_trt.engine";
    descTrt.backendType = DetectionBackendType::TensorRt;
    mgr.registerDetectorDescriptor(descTrt);

    ImageView img{};
    img.width = 0;
    img.height = 0;
    img.stride = 0;
    img.pixelFormat = "RGB8";

    DetectionResult result;

    auto rknnBackend = mgr.createDetector("yolo_rknn");
    assert(rknnBackend && rknnBackend->isLoaded());
    assert(rknnBackend->run(img, result));

    auto trtBackend = mgr.createDetector("yolo_trt");
    assert(trtBackend && trtBackend->isLoaded());
    assert(trtBackend->run(img, result));
}

void test_detector_config_loader_from_yaml() {
    using namespace falconmind::sdk::perception;

    // 在当前工作目录下创建一个临时 YAML 配置文件
    const char* fileName = "detectors_test.yaml";
    {
        std::ofstream ofs(fileName);
        ofs << "detectors:\n"
            << "  - id: yolo_v26_640_onnx\n"
            << "    name: YOLOv26 640 ONNX\n"
            << "    model_path: /opt/models/yolo_v26_640.onnx\n"
            << "    label_path: /opt/models/coco80.txt\n"
            << "    backend: onnxruntime\n"
            << "    device: cpu\n"
            << "    device_index: 0\n"
            << "    precision: fp32\n"
            << "    input_width: 640\n"
            << "    input_height: 640\n"
            << "    num_classes: 80\n"
            << "    score_threshold: 0.25\n"
            << "    nms_threshold: 0.45\n";
    }

    PerceptionPluginManager mgr;
    mgr.registerDetectorBackend("onnxruntime",
                                DetectionBackendType::OnnxRuntime,
                                []() { return std::make_shared<OnnxRuntimeDetectorBackend>(); });

    bool ok = loadDetectorsFromYamlFile(fileName, mgr);
    assert(ok);

    auto detectors = mgr.listDetectors();
    assert(!detectors.empty());

    auto backend = mgr.createDetector("yolo_v26_640_onnx");
    assert(backend && backend->isLoaded());

    ImageView img{};
    img.width = 0;
    img.height = 0;
    img.stride = 0;
    img.pixelFormat = "RGB8";

    DetectionResult result;
    assert(backend->run(img, result));
}

void test_simple_tracker_backend_and_tracking_node() {
    using namespace falconmind::sdk::perception;

    // 1) 直接测试 SimpleTrackerBackend
    SimpleTrackerBackend tracker;
    assert(tracker.load());

    DetectionResult dets;
    dets.frameId = "frame0";
    dets.timestampNs = 0;
    dets.frameIndex = 0;

    Detection d;
    d.bbox = {0.0f, 0.0f, 50.0f, 50.0f};
    d.score = 0.8f;
    d.classId = 1;
    d.className = "car";
    dets.detections.push_back(d);

    TrackingResult tracks;
    assert(tracker.run(dets, tracks));
    assert(!tracks.tracks.empty());
    assert(dets.detections[0].trackId > 0);

    // 2) 测试 TrackingTransformNode 与后端组合调用（当前使用内部 dummy 检测）
    auto backendPtr = std::make_shared<SimpleTrackerBackend>();
    assert(backendPtr->load());

    TrackingTransformNode node;
    node.setBackend(backendPtr);
    assert(node.start());
    node.process(); // 日志中可看到 tracks 数量
}

void test_environment_detection_node_output() {
    using namespace falconmind::sdk::perception;

    EnvironmentDetectionNode node;
    assert(node.start());
    auto outPad = node.getPad("env_status_out");
    assert(outPad);
    std::vector<uint8_t> received;
    auto sinkPad = std::make_shared<Pad>("sink", PadType::Sink);
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        received.assign(p, p + size);
    });
    assert(outPad->connectTo(sinkPad, "sink", "sink"));
    node.process();
    assert(received.size() == sizeof(EnvironmentStatusPacket));
    const auto* pkt = reinterpret_cast<const EnvironmentStatusPacket*>(received.data());
    assert(pkt->state == static_cast<int32_t>(EnvironmentState::Normal));
    assert(pkt->confidence >= 0.f && pkt->confidence <= 1.f);
}

void test_low_light_adaptation_gamma() {
    using namespace falconmind::sdk::perception;
    using namespace falconmind::sdk::sensors;

    LowLightAdaptationNode node;
    node.setBrightnessThreshold(200);
    node.setGamma(1.5f);
    assert(node.start());
    std::vector<uint8_t> received;
    auto outPad = node.getPad("image_out");
    auto inPad = node.getPad("image_in");
    assert(outPad && inPad);
    auto sinkPad = std::make_shared<Pad>("sink", PadType::Sink);
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        received.assign(p, p + size);
    });
    assert(outPad->connectTo(sinkPad, "sink", "in"));
    auto srcPad = std::make_shared<Pad>("src", PadType::Source);
    assert(srcPad->connectTo(inPad, "low_light", "image_in"));
    CameraFramePacket header;
    header.width = 4;
    header.height = 4;
    header.stride = 12;
    std::strncpy(header.format, "RGB8", sizeof(header.format) - 1);
    header.format[sizeof(header.format) - 1] = '\0';
    std::vector<uint8_t> frame(sizeof(CameraFramePacket) + 4 * 4 * 3, 0);
    *reinterpret_cast<CameraFramePacket*>(frame.data()) = header;
    uint8_t* pix = frame.data() + sizeof(CameraFramePacket);
    for (int i = 0; i < 4 * 4 * 3; ++i) pix[i] = 40;
    srcPad->pushToConnections(frame.data(), frame.size());
    node.process();
    assert(received.size() == frame.size());
    const uint8_t* outPix = received.data() + sizeof(CameraFramePacket);
    assert(outPix[0] > 40);
}

void test_visual_slam_node_default_pose() {
    using namespace falconmind::sdk::perception;

    VisualSlamNode node;
    assert(node.start());
    std::vector<uint8_t> received;
    auto sinkPad = std::make_shared<Pad>("sink", PadType::Sink);
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        received.assign(p, p + size);
    });
    assert(node.getPad("pose_out")->connectTo(sinkPad, "sink", "in"));
    node.process();
    assert(received.size() == sizeof(Pose3D));
    const auto* pose = reinterpret_cast<const Pose3D*>(received.data());
    assert(pose->x == 0. && pose->y == 0. && pose->z == 0.);
    assert(pose->qx == 0. && pose->qy == 0. && pose->qz == 0. && pose->qw == 1.);
}

void test_gnss_source_sim() {
    using namespace falconmind::sdk::sensors;

    GnssSourceNode node;
    node.setSimulatedFix(31.2, 121.5, 10.0);
    assert(node.start());
    std::vector<uint8_t> received;
    auto sinkPad = std::make_shared<Pad>("sink", PadType::Sink);
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        received.assign(p, p + size);
    });
    assert(node.getPad("gnss_out")->connectTo(sinkPad, "sink", "in"));
    node.process();
    assert(received.size() == sizeof(GnssSample));
    const auto* s = reinterpret_cast<const GnssSample*>(received.data());
    assert(s->latitude == 31.2 && s->longitude == 121.5 && s->altitude == 10.0);
}

void test_imu_source_sim() {
    using namespace falconmind::sdk::sensors;

    ImuSourceNode node;
    assert(node.start());
    std::vector<uint8_t> received;
    auto sinkPad = std::make_shared<Pad>("sink", PadType::Sink);
    sinkPad->setDataCallback([&received](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        received.assign(p, p + size);
    });
    assert(node.getPad("imu_out")->connectTo(sinkPad, "sink", "in"));
    node.process();
    assert(received.size() == sizeof(ImuSample));
    const auto* s = reinterpret_cast<const ImuSample*>(received.data());
    assert(s->az > 9.0 && s->az < 10.0);
}

void test_slam_service_client_from_file() {
    using namespace falconmind::sdk::perception;

    std::string path = "/tmp/falconmind_pose_test.bin";
    Pose3D written;
    written.x = 1.0;
    written.y = 2.0;
    written.z = 3.0;
    written.qw = 1.0;
    written.timestampNs = 12345;
    {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(&written), sizeof(Pose3D));
    }
    SlamServiceClientFromFile client(path);
    assert(client.isAvailable());
    Pose3D read;
    assert(client.getPose(read));
    assert(read.x == 1.0 && read.y == 2.0 && read.z == 3.0);
    assert(read.qw == 1.0 && read.timestampNs == 12345);
}

} // namespace

int main() {
    std::cout << "[core_pipeline_tests] Running tests..." << std::endl;

    test_pipeline_add_and_link();
    test_node_and_pad_basic();
    test_pad_push_to_connections();
    test_camera_frame_packet();
    test_caps_properties();
    test_bus_publish_subscribe();
    test_flight_connection_service_basic();
    test_flight_nodes_basic();
    test_camera_source_node_basic();
    test_camera_to_detection_pipeline();
    test_perception_plugin_manager_with_onnx_backend();
    test_perception_plugin_manager_with_rknn_and_tensorrt();
    test_detector_config_loader_from_yaml();
    test_simple_tracker_backend_and_tracking_node();
    test_environment_detection_node_output();
    test_low_light_adaptation_gamma();
    test_visual_slam_node_default_pose();
    test_gnss_source_sim();
    test_imu_source_sim();
    test_slam_service_client_from_file();

    std::cout << "[core_pipeline_tests] All tests passed." << std::endl;
    return 0;
}

