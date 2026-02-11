// FalconMindSDK Python Bindings using pybind11
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/chrono.h>
#include <sstream>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/mission/EventReporterNode.h"
#include "falconmind/sdk/mission/SearchTypes.h"

namespace py = pybind11;
using namespace falconmind::sdk;

PYBIND11_MODULE(falconmind_sdk, m) {
    m.doc() = "FalconMind SDK Python Bindings";

    // PipelineState enum
    py::enum_<core::PipelineState>(m, "PipelineState")
        .value("Null", core::PipelineState::Null)
        .value("Ready", core::PipelineState::Ready)
        .value("Playing", core::PipelineState::Playing)
        .value("Paused", core::PipelineState::Paused);

    // PipelineConfig
    py::class_<core::PipelineConfig>(m, "PipelineConfig")
        .def(py::init<>())
        .def_readwrite("pipeline_id", &core::PipelineConfig::pipelineId)
        .def_readwrite("name", &core::PipelineConfig::name)
        .def_readwrite("description", &core::PipelineConfig::description);

    // Pipeline::LinkInfo
    py::class_<core::Pipeline::LinkInfo>(m, "LinkInfo")
        .def_readwrite("src_node_id", &core::Pipeline::LinkInfo::srcNodeId)
        .def_readwrite("src_pad_name", &core::Pipeline::LinkInfo::srcPadName)
        .def_readwrite("dst_node_id", &core::Pipeline::LinkInfo::dstNodeId)
        .def_readwrite("dst_pad_name", &core::Pipeline::LinkInfo::dstPadName);

    // Pipeline
    py::class_<core::Pipeline, std::shared_ptr<core::Pipeline>>(m, "Pipeline")
        .def(py::init<const core::PipelineConfig&>())
        .def("id", &core::Pipeline::id)
        .def("add_node", &core::Pipeline::addNode)
        .def("link", &core::Pipeline::link)
        .def("unlink", &core::Pipeline::unlink)
        .def("set_state", &core::Pipeline::setState)
        .def("state", &core::Pipeline::state)
        .def("get_node", &core::Pipeline::getNode)
        .def("get_links", &core::Pipeline::getLinks);

    // Node (base class)
    py::class_<core::Node, std::shared_ptr<core::Node>>(m, "Node")
        .def("id", &core::Node::id)
        .def("set_id", &core::Node::setId)
        .def("start", &core::Node::start)
        .def("stop", &core::Node::stop)
        .def("process", &core::Node::process)
        .def("configure", &core::Node::configure);

    // NodeFactory
    py::class_<core::NodeFactory>(m, "NodeFactory")
        .def_static("register_node_type", &core::NodeFactory::registerNodeType,
                    py::arg("template_id"), py::arg("creator"))
        .def_static("create_node", [](const std::string& template_id, const std::string& node_id, py::dict params = py::dict()) {
            // Convert Python dict to void* (we'll pass nullptr for now)
            return core::NodeFactory::createNode(template_id, node_id, nullptr);
        }, py::arg("template_id"), py::arg("node_id"), py::arg("params") = py::dict())
        .def_static("is_registered", &core::NodeFactory::isRegistered)
        .def_static("get_registered_types", &core::NodeFactory::getRegisteredTypes)
        .def_static("initialize_default_types", &core::NodeFactory::initializeDefaultTypes);

    // FlowExecutor
    py::class_<core::FlowExecutor>(m, "FlowExecutor")
        .def(py::init<>())
        .def("load_flow", &core::FlowExecutor::loadFlow)
        .def("load_flow_from_file", &core::FlowExecutor::loadFlowFromFile)
        .def("load_flow_from_builder", &core::FlowExecutor::loadFlowFromBuilder)
        .def("start", &core::FlowExecutor::start)
        .def("stop", &core::FlowExecutor::stop)
        .def("is_running", &core::FlowExecutor::isRunning)
        .def("get_pipeline", &core::FlowExecutor::getPipeline)
        .def("get_flow_id", &core::FlowExecutor::getFlowId)
        .def("get_flow_name", &core::FlowExecutor::getFlowName)
        .def("update_flow", &core::FlowExecutor::updateFlow);

    // GeoPoint
    py::class_<mission::GeoPoint>(m, "GeoPoint")
        .def(py::init<>())
        .def(py::init<double, double, double>(), py::arg("lat"), py::arg("lon"), py::arg("alt"))
        .def_readwrite("lat", &mission::GeoPoint::lat)
        .def_readwrite("lon", &mission::GeoPoint::lon)
        .def_readwrite("alt", &mission::GeoPoint::alt);

    // SearchPattern enum
    py::enum_<mission::SearchPattern>(m, "SearchPattern")
        .value("LAWN_MOWER", mission::SearchPattern::LAWN_MOWER)
        .value("SPIRAL", mission::SearchPattern::SPIRAL)
        .value("ZIGZAG", mission::SearchPattern::ZIGZAG)
        .value("SECTOR", mission::SearchPattern::SECTOR)
        .value("WAYPOINT_LIST", mission::SearchPattern::WAYPOINT_LIST);

    // SearchArea
    py::class_<mission::SearchArea>(m, "SearchArea")
        .def(py::init<>())
        .def_readwrite("polygon", &mission::SearchArea::polygon)
        .def_readwrite("min_altitude", &mission::SearchArea::minAltitude)
        .def_readwrite("max_altitude", &mission::SearchArea::maxAltitude);

    // SearchParams
    py::class_<mission::SearchParams>(m, "SearchParams")
        .def(py::init<>())
        .def_readwrite("pattern", &mission::SearchParams::pattern)
        .def_readwrite("altitude", &mission::SearchParams::altitude)
        .def_readwrite("speed", &mission::SearchParams::speed)
        .def_readwrite("spacing", &mission::SearchParams::spacing)
        .def_readwrite("loiter_time", &mission::SearchParams::loiterTime)
        .def_readwrite("enable_detection", &mission::SearchParams::enableDetection)
        .def_readwrite("detection_classes", &mission::SearchParams::detectionClasses);

    // SearchPathPlannerNode
    py::class_<mission::SearchPathPlannerNode, core::Node, std::shared_ptr<mission::SearchPathPlannerNode>>(m, "SearchPathPlannerNode")
        .def(py::init<>())
        .def("set_search_area", &mission::SearchPathPlannerNode::setSearchArea)
        .def("set_search_params", &mission::SearchPathPlannerNode::setSearchParams)
        .def("get_waypoints", &mission::SearchPathPlannerNode::getWaypoints);

    // EventReporterNode
    py::class_<mission::EventReporterNode, core::Node, std::shared_ptr<mission::EventReporterNode>>(m, "EventReporterNode")
        .def(py::init<>())
        .def("configure", &mission::EventReporterNode::configure);
}
