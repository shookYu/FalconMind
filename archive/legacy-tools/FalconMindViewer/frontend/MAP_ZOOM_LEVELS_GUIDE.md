# 地图缩放级别清晰度指南

> **创建日期**: 2024-02-01

## 📊 OpenStreetMap 缩放级别说明

### 标准缩放级别范围

OpenStreetMap 的标准瓦片服务支持 **Zoom 0-19** 级别，但不同级别的清晰度和可用性不同：

| 缩放级别 | 清晰度 | 应用场景 | 瓦片分辨率 | 典型用途 |
|---------|--------|---------|-----------|---------|
| **0-6** | ⭐ 很低 | 世界/大陆级别 | 粗糙 | 全球概览 |
| **7-10** | ⭐⭐ 低 | 国家/省份级别 | 中等 | 区域规划 |
| **11-14** | ⭐⭐⭐ 中等 | 城市级别 | 清晰 | 城市导航 |
| **15-16** | ⭐⭐⭐⭐ 高 | 街道级别 | 很清晰 | **推荐日常使用** |
| **17-18** | ⭐⭐⭐⭐⭐ 很高 | 建筑级别 | 非常清晰 | **最高清晰度** |
| **19** | ⭐⭐⭐⭐⭐ 极高 | 细节级别 | 极高 | 特定区域（可能不完整） |

## 🎯 最清晰级别推荐

### **Zoom 17-18** 是最清晰的实用级别

**原因**：
1. ✅ **清晰度最高**：可以看到街道名称、建筑轮廓、地标细节
2. ✅ **数据完整**：OpenStreetMap 在大多数地区都有完整数据
3. ✅ **性能可接受**：虽然瓦片数量多，但现代浏览器可以流畅显示
4. ✅ **实用性强**：适合无人机任务规划、详细导航等应用

### **Zoom 19** 级别说明

- ⚠️ **数据不完整**：不是所有地区都有 zoom 19 的瓦片
- ⚠️ **性能开销大**：瓦片数量巨大（zoom 19 是 zoom 18 的 4 倍）
- ⚠️ **实际收益有限**：相比 zoom 18，清晰度提升不明显

## 📐 不同级别的实际分辨率

### 每个瓦片的实际覆盖范围

- **Zoom 0**: 1 个瓦片 = 整个地球
- **Zoom 10**: 1 个瓦片 ≈ 150 km × 150 km
- **Zoom 14**: 1 个瓦片 ≈ 9.5 km × 9.5 km
- **Zoom 16**: 1 个瓦片 ≈ 2.4 km × 2.4 km
- **Zoom 18**: 1 个瓦片 ≈ 0.6 km × 0.6 km
- **Zoom 19**: 1 个瓦片 ≈ 0.3 km × 0.3 km

### 像素分辨率（假设 256×256 像素/瓦片）

- **Zoom 14**: 约 27 米/像素
- **Zoom 16**: 约 9.5 米/像素
- **Zoom 18**: 约 2.4 米/像素 ⭐ **推荐**
- **Zoom 19**: 约 1.2 米/像素

## 💡 实际使用建议

### 1. 日常使用推荐

```bash
# 下载 Zoom 0-16 级别（平衡清晰度和性能）
python3 download_map_tiles.py --beijing --min-zoom 0 --max-zoom 16 --output tiles --workers 5 --delay 0.2
```

**优点**：
- ✅ 清晰度足够（可以看到街道和主要建筑）
- ✅ 瓦片数量适中（约 10-20 万个瓦片）
- ✅ 下载时间合理（几小时到一天）
- ✅ 性能流畅

### 2. 最高清晰度（推荐）

```bash
# 下载 Zoom 0-18 级别（最高清晰度）
python3 download_map_tiles.py --beijing --min-zoom 0 --max-zoom 18 --output tiles --workers 5 --delay 0.2
```

**优点**：
- ✅ 清晰度最高（可以看到建筑细节）
- ✅ 适合无人机任务规划
- ⚠️ 瓦片数量多（约 50-100 万个瓦片）
- ⚠️ 下载时间长（可能需要数天）

### 3. 分阶段下载策略

```bash
# 第一阶段：快速下载基础级别（0-14）
python3 download_map_tiles.py --beijing --min-zoom 0 --max-zoom 14 --output tiles --workers 10 --delay 0.1

# 第二阶段：补充高清晰度级别（15-18）
python3 download_map_tiles.py --beijing --min-zoom 15 --max-zoom 18 --output tiles --workers 5 --delay 0.2
```

## 📈 瓦片数量估算（北京市区域）

| 缩放级别 | 瓦片数量 | 存储空间（估算） | 下载时间（估算） |
|---------|---------|----------------|----------------|
| 0-14 | ~3,800 | ~75 MB | ~13 分钟 |
| 0-16 | ~60,000 | ~1.2 GB | ~3 小时 |
| 0-18 | ~960,000 | ~19 GB | ~2-3 天 |
| 0-19 | ~3,840,000 | ~77 GB | ~1-2 周 |

**注意**：实际数值可能因区域和网络状况而异。

## 🔧 配置 Viewer 使用不同级别

### 当前配置（Zoom 14）

```javascript
// app.js
osmImagery = new Cesium.UrlTemplateImageryProvider({
  url: localTilesPath,
  maximumLevel: 14,  // 当前设置为 14
  minimumLevel: 0,
});
```

### 推荐配置（Zoom 18）

```javascript
// app.js
osmImagery = new Cesium.UrlTemplateImageryProvider({
  url: localTilesPath,
  maximumLevel: 18,  // 改为 18 获得最高清晰度
  minimumLevel: 0,
});
```

**重要**：`maximumLevel` 必须与下载的最大缩放级别一致，否则会请求不存在的瓦片。

## 🎨 清晰度对比示例

### Zoom 14（当前）
- 可以看到主要街道
- 可以看到主要建筑轮廓
- 文字可能不够清晰

### Zoom 16（推荐日常使用）
- 可以看到详细街道
- 可以看到建筑细节
- 文字清晰可读
- 性能流畅

### Zoom 18（最高清晰度）
- 可以看到每条街道
- 可以看到建筑细节和轮廓
- 文字非常清晰
- 适合精确任务规划

## ⚡ 性能优化建议

### 如果使用 Zoom 18 级别

1. **增加缓存**：
```javascript
viewer.scene.globe.tileCacheSize = 10000; // 增加到 10000
```

2. **优化并发请求**：
```javascript
viewer.scene.globe._surface._tileProvider._requestScheduler.maximumRequests = 100;
```

3. **预加载策略**：
```javascript
viewer.scene.globe.preloadSiblings = true;
viewer.scene.globe.preloadAncestors = true;
```

## 📝 总结

### 最佳实践

1. **日常使用**：Zoom 0-16（平衡清晰度和性能）
2. **最高清晰度**：Zoom 0-18（推荐用于无人机任务规划）
3. **不推荐**：Zoom 19（数据不完整，性能开销大）

### 清晰度排序

**Zoom 18 > Zoom 17 > Zoom 16 > Zoom 15 > Zoom 14**

**结论**：**Zoom 18 是 OpenStreetMap 最清晰的实用级别**，适合大多数应用场景。

---

**最后更新**: 2024-02-01
