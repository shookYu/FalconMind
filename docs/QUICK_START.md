# 百日攻坚 - 快速启动指南

## 今天就开始！

### 第一步：确认团队（第1天）

- [ ] **工程师A**（SDK导航/控制）- 联系确认
- [ ] **工程师B**（SDK感知/Mission）- 联系确认
- [ ] **工程师C**（Viewer全栈）- 联系确认
- [ ] **工程师D**（Builder/测试）- 联系确认

### 第二步：环境准备（第1-2天）

**所有工程师执行：**

```bash
# 1. 克隆仓库
git clone https://github.com/shookYu/FalconMind.git
cd FalconMind

# 2. 安装依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git \
    libopencv-dev libeigen3-dev libyaml-cpp-dev \
    python3-pip nodejs npm

# 3. 验证PoC配置
cat PoC/Scenario_01_DeniedGPS_VisualTracking/README.md

# 4. 阅读需求清单
cat PoC/Scenario_01_DeniedGPS_VisualTracking/docs/requirements_gap_list.md
```

### 第三步：创建开发分支（第2天）

```bash
# 工程师A
git checkout -b feature/sdk-flow-nodes

# 工程师B  
git checkout -b feature/sdk-mission-system

# 工程师C
git checkout -b feature/viewer-mission-api

# 工程师D
git checkout -b feature/builder-templates
```

---

## 本周任务（第1周）

### 工程师A - SDK基础 + 导航节点

**Day 1-2: 环境搭建**
```bash
# 1. 熟悉SDK结构
cd FalconMindSDK
ls -la include/ src/

# 2. 编译现有代码
mkdir build && cd build
cmake ..
make -j4

# 3. 运行现有测试
ctest
```

**Day 3-5: 实现FlowNode基类**
- [ ] 创建 `include/falconmind/sdk/flow/flow_node.hpp`
- [ ] 定义节点生命周期接口（initialize/execute/stop）
- [ ] 实现参数配置接口
- [ ] 编写单元测试

**Day 6-7: 实现VINS节点**
- [ ] VINSStatusCheckNode
- [ ] VINSInitializerNode
- [ ] 单元测试

**提交里程碑：**
```bash
git add .
git commit -m "feat(sdk): add FlowNode base class and VINS nodes

- Add FlowNode interface with lifecycle management
- Implement VINSStatusCheckNode for checking VINS readiness
- Implement VINSInitializerNode with progress reporting
- Add unit tests for all nodes

Refs: BATTLE_PLAN_100_DAYS Phase 1"
git push origin feature/sdk-flow-nodes
```

### 工程师B - SDK基础 + 感知节点

**Day 1-2: 环境搭建**
- [ ] 熟悉YOLO/DeepSORT现有代码
- [ ] 编译运行检测测试

**Day 3-5: DeepSORT完善**
- [ ] 集成OSNet特征提取
- [ ] 优化跟踪算法
- [ ] 性能测试（RK3588上<10ms）

**Day 6-7: 实现感知节点**
- [ ] VisualDetectorNode（YOLO+DeepSORT）
- [ ] TargetDetectionCheckerNode
- [ ] 单元测试

### 工程师C - Viewer基础 + API框架

**Day 1-2: 环境搭建**
```bash
cd FalconMindViewer/backend
pip install -r requirements.txt

cd ../frontend
npm install
```

**Day 3-5: API框架搭建**
- [ ] 创建Mission API路由框架
- [ ] 定义Pydantic模型
- [ ] 实现基础CRUD

**Day 6-7: Mission创建API**
- [ ] POST /missions/denied-env
- [ ] 配置验证
- [ ] 单元测试

### 工程师D - Builder基础 + 测试框架

**Day 1-2: 环境搭建**
```bash
cd FalconMindBuilder
docker-compose up -d
```

**Day 3-5: 测试框架搭建**
- [ ] 创建SDK测试框架
- [ ] 创建集成测试脚本
- [ ] 设置CI/CD基础

**Day 6-7: Flow模板机制**
- [ ] 设计模板存储结构
- [ ] 实现模板加载API
- [ ] 基础UI组件

---

## 每周检查点

### 每周五下午4点：站会

**议程（30分钟）：**
1. 每人2分钟：本周完成内容
2. 技术讨论10分钟：遇到的问题
3. 计划对齐10分钟：下周任务调整
4. 风险识别5分钟：是否有延期风险

### 每周五下班前：提交周报

在 `docs/weekly_reports/week_X.md` 提交周报：

```markdown
# 第X周周报

## 完成内容
1. 
2. 

## 进度状态
- 整体进度: X%
- 状态: 🟢 正常 / 🟡 有风险 / 🔴 延期

## 风险项
| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
|     |      |         |

## 下周计划
1. 
2. 

## 需要的支持
- 
```

---

## 关键里程碑检查

### 第3周末检查（第21天）

**工程师A检查清单：**
- [ ] FlowNode基类编译通过
- [ ] VINS节点单元测试通过
- [ ] 代码Review通过

**工程师B检查清单：**
- [ ] DeepSORT优化完成
- [ ] VisualDetectorNode编译通过
- [ ] 检测率>90%

**工程师C检查清单：**
- [ ] Viewer API框架搭建完成
- [ ] Mission创建API可用
- [ ] 基础前端页面可访问

**工程师D检查清单：**
- [ ] 测试框架搭建完成
- [ ] CI/CD基础流程可用
- [ ] Builder模板机制设计完成

**如果未完成：**
- 立即调整计划
- 加班或调整后续任务
- 确保不影响第7周里程碑

### 第7周末检查（第49天）

**所有P0节点必须完成！**

运行检查脚本：
```bash
./scripts/check_milestone_week7.sh

# 期望输出:
# ✅ VINSStatusCheckNode: PASS
# ✅ VINSInitializerNode: PASS
# ✅ GPSDefenseActivatorNode: PASS
# ✅ VisualDetectorNode: PASS
# ✅ SearchPatternGeneratorNode: PASS
# ✅ TargetDetectionCheckerNode: PASS
# ✅ VisualServoControllerNode: PASS
# ✅ TargetAwaiterNode: PASS
# 
# 总计: 8/8 PASS
```

### 第10周末检查（第70天）

**Mission系统必须可用！**

```bash
./scripts/check_milestone_week10.sh

# 期望输出:
# ✅ Mission YAML解析: PASS
# ✅ MissionExecutor: PASS
# ✅ NodeAgent Mission支持: PASS
# ✅ Viewer Mission API: PASS
```

### 第14周末检查（第98天）

**最终验收！**

```bash
./scripts/final_validation.sh

# 期望输出:
# ═══════════════════════════════════════
# FalconMind 百日攻坚最终验收
# ═══════════════════════════════════════
# 
# ✅ Builder方式: PASS
#    - Flow编排: OK
#    - 一键部署: OK
#    - 遥测显示: OK
# 
# ✅ Viewer方式: PASS
#    - Mission创建: OK
#    - 目标选择: OK
#    - 集群监控: OK
# 
# ✅ SDK方式: PASS
#    - 编译通过: OK
#    - 配置文件驱动: OK
#    - 20Hz控制: OK
# 
# 性能指标:
# - 控制频率: 20Hz ✓
# - 延迟: 79ms ✓
# - 检测率: 92% ✓
# - ID保持: 96% ✓
# 
# ═══════════════════════════════════════
# 🎉 验收通过！三种方式全部100%可用！
# ═══════════════════════════════════════
```

---

## 常见问题

### Q: 进度落后了怎么办？

**Week 1-3落后：**
- 周末加班补齐
- 简化非核心功能

**Week 4-7落后：**
- 申请人力支持
- 部分功能降级（P1变P2）

**Week 8-10落后：**
- SDK方式优先保证
- Builder/Viewer后续版本支持

### Q: 技术难点卡住怎么办？

1. **立即在群里求助**（2小时内响应）
2. **查阅相关论文/开源实现**
3. **简化方案**，先保证可用再优化
4. **如果实在不行，调整方案**，不要硬撑

### Q: 和其他工程师代码冲突怎么办？

1. **每天下班前提交代码**（避免大冲突）
2. **冲突时及时沟通**，不要擅自修改
3. **使用feature分支**，不要直接改master
4. **Code Review时统一风格**

---

## 紧急联系

| 角色 | 姓名 | 联系方式 | 负责内容 |
|-----|------|---------|---------|
| 技术负责人 | TBD | TBD | 技术决策、难点攻关 |
| 项目经理 | TBD | TBD | 进度管理、资源协调 |
| 工程师A | TBD | TBD | SDK导航/控制节点 |
| 工程师B | TBD | TBD | SDK感知/Mission系统 |
| 工程师C | TBD | TBD | Viewer开发 |
| 工程师D | TBD | TBD | Builder/测试/集成 |

---

## 快速参考

### 常用命令

```bash
# 编译SDK
cd FalconMindSDK/build && make -j4

# 运行测试
ctest --output-on-failure

# 启动Builder
cd FalconMindBuilder && docker-compose up -d

# 启动Viewer
cd FalconMindViewer && ./start-dev.sh

# 查看日志
tail -f /var/log/falconmind/*.log

# 提交代码
git add .
git commit -m "type(scope): description"
git push origin feature/xxx
```

### 关键文档

| 文档 | 路径 | 用途 |
|-----|------|------|
| 百日攻坚计划 | `docs/BATTLE_PLAN_100_DAYS.md` | 完整计划 |
| 快速启动指南 | `docs/QUICK_START.md` | 本文件 |
| PoC场景 | `PoC/Scenario_01_DeniedGPS_VisualTracking/README.md` | 需求参考 |
| 需求清单 | `PoC/docs/requirements_gap_list.md` | 开发清单 |
| 验收标准 | `PoC/docs/capability_verification_checklist.md` | 测试标准 |

---

## 激励

**第7周里程碑达成：** 团队聚餐 🎉

**第14周最终验收：** 项目奖金 + 庆功宴 🎊

**个人突出贡献：** 特别奖励 💰

---

## 开始吧！

今天是第1天，100天后我们一起庆祝胜利！

```
    🎯 目标: 三种方式100%可用
    📅 工期: 100天
    👥 团队: 4名工程师
    🚀 开始: 今天！
```

**Go Go Go! 🚀🚀🚀**
