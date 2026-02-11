// FalconMindSDK - C API（extern "C"），供非 C++ 调用方（如 NodeAgent/脚本）使用
// PRD 3.1.2.4 多语言支持：C、C++、Python。本头文件与实现提供 C 接口。
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/** 不透明句柄：Pipeline */
typedef struct FMPipeline FMPipeline;

/** 不透明句柄：FlowExecutor */
typedef struct FMFlowExecutor FMFlowExecutor;

/** Pipeline 状态：0=Null, 1=Ready, 2=Playing, 3=Paused */
typedef int FMPipelineState;
#define FM_PIPELINE_STATE_NULL    0
#define FM_PIPELINE_STATE_READY   1
#define FM_PIPELINE_STATE_PLAYING 2
#define FM_PIPELINE_STATE_PAUSED  3

// ---------- Pipeline ----------

/** 创建 Pipeline；pipeline_id、name 可为 NULL（使用默认）。调用方负责 fm_pipeline_destroy。 */
FMPipeline* fm_pipeline_create(const char* pipeline_id, const char* name);

/** 销毁 Pipeline */
void fm_pipeline_destroy(FMPipeline* p);

/** 添加节点：template_id 如 "camera_source","dummy_detection","search_path_planner" 等，node_id 为实例 ID。返回 1 成功，0 失败。 */
int fm_pipeline_add_node(FMPipeline* p, const char* template_id, const char* node_id);

/** 连接 Pad。返回 1 成功，0 失败。 */
int fm_pipeline_link(FMPipeline* p,
    const char* src_node_id, const char* src_pad_name,
    const char* dst_node_id, const char* dst_pad_name);

/** 设置状态。返回 1 成功，0 失败。 */
int fm_pipeline_set_state(FMPipeline* p, FMPipelineState state);

/** 获取当前状态 */
FMPipelineState fm_pipeline_get_state(FMPipeline* p);

// ---------- FlowExecutor（零代码 Flow 执行） ----------

/** 创建 FlowExecutor。调用方负责 fm_flow_executor_destroy。 */
FMFlowExecutor* fm_flow_executor_create(void);

/** 销毁 FlowExecutor */
void fm_flow_executor_destroy(FMFlowExecutor* e);

/** 从 JSON 字符串加载 Flow。返回 1 成功，0 失败。 */
int fm_flow_executor_load_flow(FMFlowExecutor* e, const char* flow_json);

/** 从文件路径加载 Flow。返回 1 成功，0 失败。 */
int fm_flow_executor_load_flow_from_file(FMFlowExecutor* e, const char* file_path);

/** 启动 Flow 执行。返回 1 成功，0 失败。 */
int fm_flow_executor_start(FMFlowExecutor* e);

/** 停止 Flow 执行 */
void fm_flow_executor_stop(FMFlowExecutor* e);

/** 是否正在运行：1 是，0 否 */
int fm_flow_executor_is_running(FMFlowExecutor* e);

#ifdef __cplusplus
}
#endif
