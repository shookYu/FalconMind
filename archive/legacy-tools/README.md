# 历史工具归档

本目录包含 FalconMind 项目的早期原型工具：

## 目录结构

```
legacy-tools/
├── FalconMindBuilder/     # 可视化代码生成器
└── FalconMindViewer/      # 单机 UAV 监控查看器
```

## 工具说明

### FalconMindBuilder
- **定位**: 可视化业务流程构建工具
- **功能**: 拖拽节点生成 C++ SDK 工程代码
- **特点**: 零代码/低代码，面向行业工程师
- **状态**: 原型完成（M4.2），功能已整合到 Console

### FalconMindViewer  
- **定位**: UAV 实时监控系统
- **功能**: Cesium 3D 地图显示单机 UAV 遥测
- **特点**: 轻量级，纯静态前端
- **状态**: 最小可用版（M4.1），功能已整合到 Console

## 为什么归档

FalconMindConsole 已完全替代这两个工具：

| 功能 | 旧工具 | Console |
|------|--------|---------|
| 流程编排 | Builder 生成代码 | Console 运行时下发 |
| 监控查看 | Viewer 单机显示 | Console 多机管理 |
| 用户认证 | 无 | JWT 完整认证 |
| 部署方式 | 手动配置 | Docker 一键启动 |

## 保留价值

1. **历史参考**: 理解项目演进过程
2. **文档查阅**: 设计文档仍有参考价值
3. **轻量演示**: Viewer 可直接运行演示单机监控

## 使用建议

**新项目请直接使用 FalconMindConsole**

如需运行旧工具：
```bash
cd FalconMindBuilder/
# 查看各自的 README.md 启动
```

## 相关链接

- [FalconMindConsole](../FalconMindConsole/) - 当前主项目
- [项目根目录](../../README.md) - 总体说明

---

**归档时间**: 2026-03-05  
**替代版本**: FalconMindConsole v1.0
