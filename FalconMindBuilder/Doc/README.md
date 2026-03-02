# FalconMindBuilder 文档目录

## 📖 阅读顺序指南

本文档目录包含 FalconMindBuilder 的完整设计文档，建议按以下顺序阅读：

### 🎯 快速导航

| 序号 | 文档 | 内容概要 | 建议阅读时间 |
|------|------|----------|-------------|
| **01** | [架构关系说明](./01_Architecture_Relationship.md) | **必读**：系统架构关系、组件定位、配置解释执行架构 | 15分钟 |
| **02** | [可行性分析](./02_Feasibility_Analysis.md) | 技术分层、可行性评估、推荐架构 | 20分钟 |
| **03** | [架构设计](./03_Architecture_Design.md) | 系统架构图、核心模块设计、数据流设计 | 30分钟 |
| **04** | [快速开始](./04_QuickStart.md) | 5分钟上手、核心功能详解、最佳实践 | 25分钟 |
| **05** | [技术细节 Part 1](./05_Technical_Details_Part1.md) | 三层抽象策略、画布编辑器实现 | 40分钟 |
| **06** | [技术细节 Part 2](./06_Technical_Details_Part2.md) | 配置到代码转换、实时预览系统 | 45分钟 |
| **07** | [技术细节 Part 3](./07_Technical_Details_Part3.md) | 插件化业务模板系统 | 35分钟 |
| **08** | [技术细节 Part 4](./08_Technical_Details_Part4.md) | SDK集成、BS架构、MVP计划 | 40分钟 |

---

## 📚 按角色阅读

### 👨‍💼 产品经理 / 决策者
**推荐阅读**：01 → 02 → 03

重点了解：
- 产品定位和架构关系
- 可行性评估和竞争优势
- 系统架构和关键设计决策

### 👨‍💻 架构师 / 技术负责人
**推荐阅读**：01 → 03 → 05 → 06 → 07 → 08

重点了解：
- 系统架构和组件关系
- 技术实现细节
- 与现有系统的集成方案
- BS架构部署方案

### 👨‍🎨 前端开发者
**推荐阅读**：01 → 04 → 05 → 06

重点了解：
- 画布编辑器技术实现（Vue-Flow）
- 节点组件设计
- 实时预览系统
- 表单配置引擎

### 👨‍🔧 后端开发者
**推荐阅读**：01 → 03 → 06 → 07 → 08

重点了解：
- 配置解释执行引擎
- 模板系统设计
- SDK集成方案
- 运行时配置执行

### 🚁 UAV/飞控工程师
**推荐阅读**：01 → 04 → 08

重点了解：
- 架构关系和配置执行流程
- 快速开始指南
- NodeAgent集成和SDK FlowExecutor

---

## 📁 文档结构

```
FalconMindBuilder/Doc/
├── README.md                           # 本文件：阅读指南
├── 01_Architecture_Relationship.md     # 架构关系说明（必读）
├── 02_Feasibility_Analysis.md          # 可行性分析
├── 03_Architecture_Design.md           # 架构详细设计
├── 04_QuickStart.md                    # 快速开始指南
├── 05_Technical_Details_Part1.md       # 技术细节 Part 1
├── 06_Technical_Details_Part2.md       # 技术细节 Part 2
├── 07_Technical_Details_Part3.md       # 技术细节 Part 3
└── 08_Technical_Details_Part4.md       # 技术细节 Part 4
```

---

## 🎓 学习路径建议

### 路径 1：快速了解（1小时）
适合想快速了解 FalconMindBuilder 的读者：
1. **01_Architecture_Relationship.md** - 了解架构关系（15分钟）
2. **02_Feasibility_Analysis.md** - 了解可行性和定位（20分钟）
3. **04_QuickStart.md** - 了解如何使用（25分钟）

**总时长**：约 1 小时

### 路径 2：深入技术（4小时）
适合准备实现或深度理解 FalconMindBuilder 的开发者：
1. **01_Architecture_Relationship.md** - 架构基础（15分钟）
2. **02_Feasibility_Analysis.md** - 可行性分析（20分钟）
3. **03_Architecture_Design.md** - 系统架构（30分钟）
4. **05_Technical_Details_Part1.md** - 前端实现（40分钟）
5. **06_Technical_Details_Part2.md** - 后端实现（45分钟）
6. **07_Technical_Details_Part3.md** - 模板系统（35分钟）
7. **08_Technical_Details_Part4.md** - 集成部署（40分钟）

**总时长**：约 4 小时

### 路径 3：完整掌握（6小时）
适合 FalconMindBuilder 核心开发者：
- 按顺序阅读所有 8 个文档
- 重点理解配置解释执行架构
- 深入研究技术实现细节

**总时长**：约 6 小时

---

## 🔑 核心概念速查

### 关键术语

| 术语 | 说明 |
|------|------|
| **FalconMindBuilder** | FalconMindConsole 的可视化编排模块 |
| **配置解释执行** | Builder 生成 JSON 配置，SDK FlowExecutor 直接解释执行（无编译）|
| **三层抽象** | Level 1（配置化）、Level 2（可视化编排）、Level 3（脚本扩展）|
| **NodeAgent** | UAV 边缘自主代理，接收并执行 Builder 配置 |
| **FlowExecutor** | SDK 配置解释执行引擎 |

### 关键设计决策

1. **Builder 是 Console 的模块**（不是独立产品）
2. **配置解释执行**（不是代码编译）
3. **NodeAgent 接收 JSON 配置**（不是代码）
4. **SDK FlowExecutor 解释执行**（动态创建节点）

---

## 📝 文档版本

- **版本**: v1.1
- **更新日期**: 2024-03-02
- **变更记录**:
  - 重命名 `FalconBuilder` → `FalconMindBuilder`
  - 添加阅读顺序编号前缀
  - 整理到 FalconMindBuilder/Doc 目录
  - 创建本 README 阅读指南

---

## 🔗 相关资源

- [FalconMindConsole 文档](../FalconMindConsole/docs/)
- [FalconMindSDK 文档](../FalconMindSDK/Doc/)
- [NodeAgent 文档](../FalconMindSDK/NodeAgent/README.md)
- [GitHub 仓库](https://github.com/shookYu/FalconMind)

---

> 💡 **提示**：建议先阅读 `01_Architecture_Relationship.md` 了解整体架构关系，这对理解其他文档非常重要。
