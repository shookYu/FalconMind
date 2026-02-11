/**
 * 可视化管理模块
 * 处理搜索区域、路径、检测结果、热力图等可视化
 */
function createVisualizationManager(state, viewerRef) {
  const { missions } = state;
  const searchAreaEntities = {};
  const detectionEntities = {};
  const coverageHeatmapEntities = {};
  const searchPathEntities = {};

  /**
   * 更新搜索区域可视化
   */
  function updateSearchAreaForMission(missionId, searchAreaData = null) {
    const viewer = viewerRef.current;
    if (!viewer) return;
    
    // 如果没有提供数据，尝试从任务中获取
    if (!searchAreaData && missions[missionId] && missions[missionId].payload) {
      searchAreaData = missions[missionId].payload.search_area;
    }
    
    if (!searchAreaData || !searchAreaData.polygon || searchAreaData.polygon.length < 3) {
      return; // 无效的搜索区域
    }
    
    // 移除旧的搜索区域实体（如果存在）
    if (searchAreaEntities[missionId]) {
      viewer.entities.remove(searchAreaEntities[missionId]);
      delete searchAreaEntities[missionId];
    }
    
    // 将多边形顶点转换为 Cesium 坐标
    const positions = searchAreaData.polygon.map(point => 
      Cesium.Cartesian3.fromDegrees(point.lon, point.lat, point.alt || 0)
    );
    
    // 创建多边形实体
    const entity = viewer.entities.add({
      id: `search_area_${missionId}`,
      name: `Search Area: ${missionId}`,
      polygon: {
        hierarchy: positions,
        material: Cesium.Color.YELLOW.withAlpha(0.3),
        outline: true,
        outlineColor: Cesium.Color.YELLOW,
        height: searchAreaData.min_altitude || 0,
        extrudedHeight: searchAreaData.max_altitude || 100,
      },
      label: {
        text: `Search Area: ${missionId}`,
        font: "14px sans-serif",
        fillColor: Cesium.Color.YELLOW,
        outlineColor: Cesium.Color.BLACK,
        outlineWidth: 2,
        style: Cesium.LabelStyle.FILL_AND_OUTLINE,
      },
    });
    
    searchAreaEntities[missionId] = entity;
  }

  /**
   * 更新搜索路径可视化（航点连线）
   */
  function updateSearchPath(missionId, waypointsData = null) {
    const viewer = viewerRef.current;
    if (!viewer) return;
    
    // 如果没有提供数据，尝试从任务中获取
    if (!waypointsData && missions[missionId] && missions[missionId].payload) {
      waypointsData = missions[missionId].payload.waypoints;
    }
    
    if (!waypointsData || !Array.isArray(waypointsData) || waypointsData.length < 2) {
      return; // 无效的航点数据
    }
    
    // 移除旧的搜索路径实体（如果存在）
    if (searchPathEntities[missionId]) {
      if (searchPathEntities[missionId].polyline) {
        viewer.entities.remove(searchPathEntities[missionId].polyline);
      }
      if (searchPathEntities[missionId].waypoints) {
        searchPathEntities[missionId].waypoints.forEach(wp => viewer.entities.remove(wp));
      }
      delete searchPathEntities[missionId];
    }
    
    // 将航点转换为 Cesium 坐标
    const positions = waypointsData.map(point => 
      Cesium.Cartesian3.fromDegrees(
        point.lon || point.longitude,
        point.lat || point.latitude,
        point.alt || point.altitude || 0
      )
    );
    
    // 创建航点连线（polyline）
    const polylineEntity = viewer.entities.add({
      id: `search_path_${missionId}`,
      name: `Search Path: ${missionId}`,
      polyline: {
        positions: positions,
        width: 3,
        material: Cesium.Color.CYAN.withAlpha(0.8),
        clampToGround: false,
        arcType: Cesium.ArcType.GEODESIC,
      },
    });
    
    // 创建航点标记
    const waypointEntities = [];
    waypointsData.forEach((point, index) => {
      const position = Cesium.Cartesian3.fromDegrees(
        point.lon || point.longitude,
        point.lat || point.latitude,
        point.alt || point.altitude || 0
      );
      
      const waypointEntity = viewer.entities.add({
        id: `waypoint_${missionId}_${index}`,
        name: `Waypoint ${index + 1}`,
        position: position,
        point: {
          pixelSize: 8,
          color: Cesium.Color.CYAN,
          outlineColor: Cesium.Color.WHITE,
          outlineWidth: 2,
          heightReference: Cesium.HeightReference.CLAMP_TO_GROUND,
        },
        label: {
          text: `${index + 1}`,
          font: "12px sans-serif",
          fillColor: Cesium.Color.CYAN,
          outlineColor: Cesium.Color.BLACK,
          outlineWidth: 2,
          pixelOffset: new Cesium.Cartesian2(0, -25),
          style: Cesium.LabelStyle.FILL_AND_OUTLINE,
        },
        description: `
          <div style="padding: 10px;">
            <h3>Waypoint ${index + 1}</h3>
            <p><strong>Latitude:</strong> ${(point.lat || point.latitude).toFixed(6)}</p>
            <p><strong>Longitude:</strong> ${(point.lon || point.longitude).toFixed(6)}</p>
            <p><strong>Altitude:</strong> ${(point.alt || point.altitude || 0).toFixed(1)} m</p>
          </div>
        `,
      });
      
      waypointEntities.push(waypointEntity);
    });
    
    searchPathEntities[missionId] = {
      polyline: polylineEntity,
      waypoints: waypointEntities
    };
  }

  /**
   * 更新搜索区域（从 WebSocket 消息）
   */
  function updateSearchArea(data) {
    if (!data || !data.mission_id) return;
    updateSearchAreaForMission(data.mission_id, data);
  }

  /**
   * 更新检测结果标记
   */
  function updateDetection(data) {
    const viewer = viewerRef.current;
    if (!viewer || !data) return;
    
    const detectionId = data.detection_id || `detection_${Date.now()}_${Math.random()}`;
    const position = data.position || data.geo_position;
    
    if (!position || position.lat === undefined || position.lon === undefined) {
      return; // 无效的检测结果
    }
    
    // 移除旧的检测结果实体（如果存在）
    if (detectionEntities[detectionId]) {
      viewer.entities.remove(detectionEntities[detectionId]);
      delete detectionEntities[detectionId];
    }
    
    // 创建检测结果标记
    const entity = viewer.entities.add({
      id: detectionId,
      name: `Detection: ${data.target_class || "Unknown"}`,
      position: Cesium.Cartesian3.fromDegrees(
        position.lon,
        position.lat,
        position.alt || 0
      ),
      billboard: {
        image: "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyNCIgaGVpZ2h0PSIyNCIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJyZWQiPjxwYXRoIGQ9Ik0xMiAyQzYuNDggMiAyIDYuNDggMiAxMnM0LjQ4IDEwIDEwIDEwIDEwLTQuNDggMTAtMTBTMTcuNTIgMiAxMiAyem0xIDE4aC0ydi0yaDJ2MnptMC00aC0ydi02aDJ2NnoiLz48L3N2Zz4=",
        scale: 1.0,
        color: Cesium.Color.RED,
      },
      label: {
        text: `${data.target_class || "Target"}\nConf: ${((data.confidence || 0) * 100).toFixed(1)}%`,
        font: "12px sans-serif",
        fillColor: Cesium.Color.RED,
        outlineColor: Cesium.Color.WHITE,
        outlineWidth: 2,
        pixelOffset: new Cesium.Cartesian2(0, -40),
        style: Cesium.LabelStyle.FILL_AND_OUTLINE,
      },
      description: `
        <div style="padding: 10px;">
          <h3>Detection Result</h3>
          <p><strong>Class:</strong> ${data.target_class || "Unknown"}</p>
          <p><strong>Confidence:</strong> ${((data.confidence || 0) * 100).toFixed(1)}%</p>
          <p><strong>Position:</strong> ${position.lat.toFixed(6)}, ${position.lon.toFixed(6)}</p>
          <p><strong>Altitude:</strong> ${(position.alt || 0).toFixed(1)} m</p>
          ${data.timestamp ? `<p><strong>Time:</strong> ${new Date(data.timestamp).toLocaleString()}</p>` : ""}
        </div>
      `,
    });
    
    detectionEntities[detectionId] = entity;
  }

  /**
   * 更新搜索覆盖热力图
   */
  function updateCoverageHeatmap(missionId, coverageData) {
    const viewer = viewerRef.current;
    if (!viewer) return;
    
    // 移除旧的热力图
    if (coverageHeatmapEntities[missionId]) {
      if (Array.isArray(coverageHeatmapEntities[missionId])) {
        coverageHeatmapEntities[missionId].forEach(entity => viewer.entities.remove(entity));
      } else {
        viewer.entities.remove(coverageHeatmapEntities[missionId]);
      }
      delete coverageHeatmapEntities[missionId];
    }
    
    if (!coverageData || !coverageData.coverage_points || coverageData.coverage_points.length === 0) {
      return;
    }
    
    // 创建热力图：使用多个半透明圆来表示覆盖密度
    const coveragePoints = coverageData.coverage_points;
    const maxCoverage = Math.max(...coveragePoints.map(p => p.coverage || 0));
    
    const entities = [];
    coveragePoints.forEach((point, index) => {
      const coverage = point.coverage || 0;
      const intensity = maxCoverage > 0 ? coverage / maxCoverage : 0;
      
      // 根据覆盖强度设置颜色（绿色=低，黄色=中，红色=高）
      let color;
      if (intensity < 0.33) {
        color = Cesium.Color.GREEN.withAlpha(0.3);
      } else if (intensity < 0.66) {
        color = Cesium.Color.YELLOW.withAlpha(0.5);
      } else {
        color = Cesium.Color.RED.withAlpha(0.7);
      }
      
      const entity = viewer.entities.add({
        id: `coverage_${missionId}_${index}`,
        name: `Coverage: ${(intensity * 100).toFixed(1)}%`,
        position: Cesium.Cartesian3.fromDegrees(
          point.lon,
          point.lat,
          point.alt || 0
        ),
        ellipse: {
          semiMajorAxis: point.radius || 50,
          semiMinorAxis: point.radius || 50,
          material: color,
          outline: true,
          outlineColor: color,
          height: point.alt || 0,
        },
      });
      
      entities.push(entity);
    });
    
    coverageHeatmapEntities[missionId] = entities;
  }

  /**
   * 处理搜索进度消息
   */
  function handleSearchProgress(data) {
    if (!data || !data.mission_id) return;
    
    // 更新搜索覆盖热力图
    if (data.coverage_points) {
      updateCoverageHeatmap(data.mission_id, data);
    }
    
    // 更新搜索区域（如果提供了新的覆盖信息）
    if (data.search_area) {
      updateSearchAreaForMission(data.mission_id, data.search_area);
    }
  }

  // 创建并返回管理器对象
  return {
    updateSearchAreaForMission,
    updateSearchPath,
    updateSearchArea,
    updateDetection,
    updateCoverageHeatmap,
    handleSearchProgress,
    detectionEntities
  };
}
