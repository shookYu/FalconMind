#include "falconmind/sdk/c_api/falconmind_sdk_c_api.h"
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/core/FlowExecutor.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace {

falconmind::sdk::core::PipelineState toState(FMPipelineState s) {
    switch (s) {
        case FM_PIPELINE_STATE_READY:   return falconmind::sdk::core::PipelineState::Ready;
        case FM_PIPELINE_STATE_PLAYING: return falconmind::sdk::core::PipelineState::Playing;
        case FM_PIPELINE_STATE_PAUSED:  return falconmind::sdk::core::PipelineState::Paused;
        default:                        return falconmind::sdk::core::PipelineState::Null;
    }
}

FMPipelineState fromState(falconmind::sdk::core::PipelineState s) {
    switch (s) {
        case falconmind::sdk::core::PipelineState::Ready:   return FM_PIPELINE_STATE_READY;
        case falconmind::sdk::core::PipelineState::Playing: return FM_PIPELINE_STATE_PLAYING;
        case falconmind::sdk::core::PipelineState::Paused:  return FM_PIPELINE_STATE_PAUSED;
        default:                                             return FM_PIPELINE_STATE_NULL;
    }
}

} // namespace

extern "C" {

FMPipeline* fm_pipeline_create(const char* pipeline_id, const char* name) {
    falconmind::sdk::core::PipelineConfig cfg;
    cfg.pipelineId = pipeline_id ? pipeline_id : "default";
    cfg.name = name ? name : "Pipeline";
    try {
        auto* p = new falconmind::sdk::core::Pipeline(cfg);
        return reinterpret_cast<FMPipeline*>(p);
    } catch (...) {
        return nullptr;
    }
}

void fm_pipeline_destroy(FMPipeline* p) {
    delete reinterpret_cast<falconmind::sdk::core::Pipeline*>(p);
}

int fm_pipeline_add_node(FMPipeline* p, const char* template_id, const char* node_id) {
    if (!p || !template_id || !node_id) return 0;
    auto* pipe = reinterpret_cast<falconmind::sdk::core::Pipeline*>(p);
    if (falconmind::sdk::core::NodeFactory::getRegisteredTypes().empty())
        falconmind::sdk::core::NodeFactory::initializeDefaultTypes();
    auto node = falconmind::sdk::core::NodeFactory::createNode(template_id, node_id, nullptr);
    if (!node) return 0;
    return pipe->addNode(node) ? 1 : 0;
}

int fm_pipeline_link(FMPipeline* p,
    const char* src_node_id, const char* src_pad_name,
    const char* dst_node_id, const char* dst_pad_name) {
    if (!p || !src_node_id || !src_pad_name || !dst_node_id || !dst_pad_name) return 0;
    auto* pipe = reinterpret_cast<falconmind::sdk::core::Pipeline*>(p);
    return pipe->link(src_node_id, src_pad_name, dst_node_id, dst_pad_name) ? 1 : 0;
}

int fm_pipeline_set_state(FMPipeline* p, FMPipelineState state) {
    if (!p) return 0;
    auto* pipe = reinterpret_cast<falconmind::sdk::core::Pipeline*>(p);
    return pipe->setState(toState(state)) ? 1 : 0;
}

FMPipelineState fm_pipeline_get_state(FMPipeline* p) {
    if (!p) return FM_PIPELINE_STATE_NULL;
    auto* pipe = reinterpret_cast<falconmind::sdk::core::Pipeline*>(p);
    return fromState(pipe->state());
}

FMFlowExecutor* fm_flow_executor_create(void) {
    try {
        auto* e = new falconmind::sdk::core::FlowExecutor();
        return reinterpret_cast<FMFlowExecutor*>(e);
    } catch (...) {
        return nullptr;
    }
}

void fm_flow_executor_destroy(FMFlowExecutor* e) {
    delete reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e);
}

int fm_flow_executor_load_flow(FMFlowExecutor* e, const char* flow_json) {
    if (!e || !flow_json) return 0;
    auto* ex = reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e);
    return ex->loadFlow(flow_json) ? 1 : 0;
}

int fm_flow_executor_load_flow_from_file(FMFlowExecutor* e, const char* file_path) {
    if (!e || !file_path) return 0;
    auto* ex = reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e);
    return ex->loadFlowFromFile(file_path) ? 1 : 0;
}

int fm_flow_executor_start(FMFlowExecutor* e) {
    if (!e) return 0;
    auto* ex = reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e);
    return ex->start() ? 1 : 0;
}

void fm_flow_executor_stop(FMFlowExecutor* e) {
    if (e)
        reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e)->stop();
}

int fm_flow_executor_is_running(FMFlowExecutor* e) {
    if (!e) return 0;
    return reinterpret_cast<falconmind::sdk::core::FlowExecutor*>(e)->isRunning() ? 1 : 0;
}

} // extern "C"
