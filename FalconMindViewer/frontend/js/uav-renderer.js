/**
 * UAV 渲染器
 * 处理 UAV 实体的创建、更新和轨迹管理
 */
function createUavRenderer(state, viewerRef, uavEntitiesRef, trajectoryHistoryRef, cesiumManager, firstTelemetryReceivedRef) {
  const { uavStates } = state;
  const { UAV_COLORS } = cesiumManager;

  /**
   * 创建或获取 UAV 实体
   */
  function getOrCreateUavEntity(uavId) {
    const viewer = viewerRef.current;
    const uavEntities = uavEntitiesRef.current;
    
    if (!uavEntities[uavId]) {
      const colorIndex = Object.keys(uavEntities).length % UAV_COLORS.length;
      uavEntities[uavId] = viewer.entities.add({
        id: uavId,
        name: uavId,
        position: Cesium.Cartesian3.fromDegrees(116.2317, 40.2265, 100.0),
        billboard: {
          image: "https://unpkg.com/ionicons@5.5.2/dist/svg/navigate-circle-outline.svg",
          scale: 0.8,
          color: UAV_COLORS[colorIndex],
        },
        label: {
          text: uavId,
          font: "14px sans-serif",
          fillColor: UAV_COLORS[colorIndex],
          pixelOffset: new Cesium.Cartesian2(0, -30),
        },
      });
    }
    return uavEntities[uavId];
  }

  /**
   * 更新 UAV 实体
   */
  function updateUavEntity(uavId, telemetry) {
    const viewer = viewerRef.current;
    if (!viewer) return;

    // 更新实体位置
    if (telemetry.position && telemetry.position.lat !== undefined && telemetry.position.lon !== undefined) {
      const entity = getOrCreateUavEntity(uavId);
      const alt = telemetry.position.alt || 0;
      entity.position = Cesium.Cartesian3.fromDegrees(
        telemetry.position.lon,
        telemetry.position.lat,
        alt
      );
      
      // 记录历史轨迹
      const trajectoryHistory = trajectoryHistoryRef.current;
      if (!trajectoryHistory[uavId]) {
        trajectoryHistory[uavId] = [];
      }
      const timestamp = Date.now();
      trajectoryHistory[uavId].push({
        position: { lat: telemetry.position.lat, lon: telemetry.position.lon, alt: alt },
        timestamp: timestamp
      });
      
      // 只保留配置的时间范围内的数据
      const retentionMs = (window.CONFIG?.TRAJECTORY_RETENTION_HOURS || 1) * 3600000;
      const cutoffTime = timestamp - retentionMs;
      trajectoryHistory[uavId] = trajectoryHistory[uavId].filter(
        point => point.timestamp > cutoffTime
      );
      
      // 限制轨迹点数
      if (window.CesiumHelpers) {
        const maxPoints = window.CONFIG?.MAX_TRAJECTORY_POINTS || 10000;
        const decimation = window.CONFIG?.TRAJECTORY_DECIMATION || 5;
        trajectoryHistory[uavId] = window.CesiumHelpers.limitTrajectoryPoints(
          trajectoryHistory[uavId],
          maxPoints,
          decimation
        );
      }
      
      // 更新轨迹线
      updateTrajectoryLine(uavId);

      // 首次收到 Telemetry 时，调整相机
      if (!firstTelemetryReceivedRef.current) {
        firstTelemetryReceivedRef.current = true;
        const cameraHeight = Math.max(alt + 200, 100);
        viewer.camera.flyTo({
          destination: Cesium.Cartesian3.fromDegrees(
            telemetry.position.lon,
            telemetry.position.lat,
            cameraHeight
          ),
          orientation: {
            heading: Cesium.Math.toRadians(0),
            pitch: Cesium.Math.toRadians(-45),
            roll: 0.0,
          },
          duration: 2.0,
        });
      }
    }
  }

  /**
   * 更新轨迹线
   */
  function updateTrajectoryLine(uavId) {
    const viewer = viewerRef.current;
    const trajectoryHistory = trajectoryHistoryRef.current;
    const uavEntities = uavEntitiesRef.current;
    
    if (!viewer || !trajectoryHistory[uavId] || trajectoryHistory[uavId].length < 2) {
      return;
    }
    
    const entityId = `trajectory_${uavId}`;
    let entity = viewer.entities.getById(entityId);
    
    const positions = trajectoryHistory[uavId].map(point =>
      Cesium.Cartesian3.fromDegrees(point.position.lon, point.position.lat, point.position.alt)
    );
    
    if (entity) {
      entity.polyline.positions = positions;
    } else {
      const uavEntity = uavEntities[uavId];
      const color = uavEntity ? uavEntity.billboard.color : Cesium.Color.CYAN;
      
      entity = viewer.entities.add({
        id: entityId,
        name: `Trajectory: ${uavId}`,
        polyline: {
          positions: positions,
          width: 2,
          material: color.withAlpha(0.7),
          clampToGround: false,
        },
      });
    }
  }

  /**
   * 更新 UAV 遥测
   */
  function updateUavTelemetry(msg, entityBatcher) {
    const t = (msg && msg.data) ? msg.data : (msg || {});
    const uavId = t.uav_id || t.uavId || "unknown";
    
    uavStates[uavId] = t;
    
    // 使用批处理更新实体（如果可用）
    if (entityBatcher && viewerRef.current) {
      entityBatcher.queueUpdate(uavId, () => {
        updateUavEntity(uavId, t);
      });
    } else {
      // 回退到直接更新
      updateUavEntity(uavId, t);
    }
  }

  /**
   * 选择 UAV
   */
  function selectUav(uavId) {
    state.selectedUavId.value = uavId;
  }

  return {
    getOrCreateUavEntity,
    updateUavEntity,
    updateUavTelemetry,
    updateTrajectoryLine,
    selectUav
  };
}
