# Viewer Backend 更新日志

## v2.0.0 - 优化版本 (2024-02-01)

### ✨ 新功能

- **模块化架构**: 代码按功能模块组织（models, services, routers, utils）
- **日志系统**: 统一的日志配置，支持文件和控制台输出，日志轮转
- **配置管理**: 使用 pydantic-settings 管理配置，支持环境变量
- **错误处理**: 全局异常处理器，详细的错误日志
- **WebSocket优化**: 消息队列、心跳检测、连接数限制
- **数据验证**: 完善的 Pydantic 模型验证
- **遥测服务优化**: 变化检测，减少不必要的网络传输

### 🔧 改进

- **代码组织**: 从单文件（500+行）重构为模块化结构
- **性能**: WebSocket 使用异步队列，避免阻塞
- **稳定性**: 完善的错误处理和数据验证
- **可维护性**: 清晰的模块划分，便于测试和维护

### 📝 变更

- **API路由**: 新版本使用 `/api/v1` 前缀
- **启动文件**: 新增 `main_optimized.py`，保留 `main.py` 向后兼容
- **依赖**: 新增 `pydantic-settings` 依赖

### 🐛 修复

- 修复 Pydantic v2 兼容性问题（`validator` → `field_validator`）

### 📚 文档

- 新增 `README_OPTIMIZATION.md` - 优化说明
- 新增 `README_STARTUP.md` - 启动说明
- 新增 `OPTIMIZATION_PROGRESS.md` - 优化进度
- 新增 `test_optimized.py` - 测试脚本

### 🔄 集成

- **启动脚本**: 已更新 `start_all_services.sh`，自动使用优化版本
- **停止脚本**: 已更新 `stop_all_services.sh`，支持停止优化版本
- **重启脚本**: 已更新 `restart_all_services.sh`，支持重启优化版本

## v1.0.0 - 初始版本

- 基础功能实现
- 单文件架构
- 基本错误处理
