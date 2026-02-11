# Viewer Backend 测试

## 运行测试

### 安装测试依赖

```bash
cd FalconMindViewer/backend
pip install -r requirements.txt
```

### 运行所有测试

```bash
pytest tests/ -v
```

### 运行特定测试文件

```bash
# 测试遥测服务
pytest tests/test_telemetry_service.py -v

# 测试数据模型
pytest tests/test_models.py -v
```

### 生成覆盖率报告

```bash
# 安装覆盖率工具
pip install pytest-cov

# 运行测试并生成覆盖率报告
pytest tests/ --cov=. --cov-report=html

# 查看HTML报告
# 打开 htmlcov/index.html
```

## 测试结构

```
tests/
├── __init__.py
├── conftest.py           # pytest配置和共享fixtures
├── test_telemetry_service.py  # 遥测服务测试
├── test_models.py       # 数据模型验证测试
└── README.md            # 本文件
```

## 测试覆盖

### 已实现测试

- ✅ 遥测服务测试
  - 首次更新
  - 无变化检测
  - 位置变化检测
  - 电池变化检测
  - 状态查询
  - 列表操作
  - 清除操作

- ✅ 数据模型验证测试
  - 有效数据验证
  - 无效数据拒绝（坐标、电池、时间戳等）
  - 边界值测试

### 待实现测试

- [ ] WebSocket管理器测试
- [ ] 路由测试
- [ ] 集成测试
- [ ] 性能测试

## 测试最佳实践

1. **单元测试**: 测试单个函数/方法
2. **集成测试**: 测试多个组件协同工作
3. **边界测试**: 测试边界值和异常情况
4. **覆盖率目标**: ≥80%
