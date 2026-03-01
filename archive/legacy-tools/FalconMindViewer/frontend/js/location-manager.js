/**
 * 位置管理模块
 * 处理位置配置加载和切换
 */
function createLocationManager(state, viewerRef, cesiumManager) {
  const { locations, selectedLocationId, defaultLocationId } = state;

  /**
   * 加载位置配置
   */
  async function loadLocations() {
    try {
      const response = await fetch("./locations.json");
      const data = await response.json();
      locations.value = data.locations || [];
      defaultLocationId.value = data.default_location || (locations.value.length > 0 ? locations.value[0].id : null);
      
      // 从localStorage恢复上次选择的位置
      const savedLocationId = localStorage.getItem("viewer_selected_location");
      if (savedLocationId && locations.value.find(loc => loc.id === savedLocationId)) {
        selectedLocationId.value = savedLocationId;
      } else {
        selectedLocationId.value = defaultLocationId.value;
      }
      
      console.log("位置配置加载成功，默认位置:", defaultLocationId.value);
    } catch (e) {
      console.error("加载位置配置失败，使用默认位置:", e);
      // 使用硬编码的默认位置
      locations.value = [{
        id: "changping_park",
        name: "北京昌平公园",
        description: "北京市昌平区昌平公园",
        lon: 116.2317,
        lat: 40.2265,
        height: 1000.0,
        heading: 0.0,
        pitch: -45.0,
        roll: 0.0
      }];
      selectedLocationId.value = "changping_park";
      defaultLocationId.value = "changping_park";
    }
  }

  /**
   * 切换到指定位置
   */
  function flyToLocation(locationId) {
    const viewer = viewerRef.current;
    if (!viewer) {
      console.warn("Cesium viewer 尚未初始化，无法切换到位置:", locationId);
      // 如果 viewer 还没初始化，延迟执行
      setTimeout(() => {
        flyToLocation(locationId);
      }, 500);
      return;
    }
    
    const location = locations.value.find(loc => loc.id === locationId);
    if (!location) {
      console.error("位置不存在:", locationId);
      return;
    }
    
    console.log(`开始切换到位置: ${location.name} (${locationId})`);
    
    selectedLocationId.value = locationId;
    // 保存到localStorage
    localStorage.setItem("viewer_selected_location", locationId);
    
    // 确保高度不会太小，防止穿模
    const EARTH_RADIUS = 6371000; // 地球半径（米）
    const MIN_DISTANCE = EARTH_RADIUS + 1000; // 最小距离：防止穿模
    
    // location.height 是从地球表面的高度，需要加上地球半径得到从地球中心的距离
    const targetHeight = (location.height || 500) + EARTH_RADIUS;
    const safeHeight = Math.max(targetHeight, MIN_DISTANCE);
    
    console.log(`目标位置: 经度=${location.lon}, 纬度=${location.lat}, 高度=${safeHeight}`);
    
    // 标记正在 flyTo，禁用自动调整
    if (cesiumManager && cesiumManager.setFlyingTo) {
      cesiumManager.setFlyingTo(true);
      console.log("已设置 isFlyingTo = true");
    } else {
      console.warn("cesiumManager.setFlyingTo 不可用");
    }
    
    try {
      viewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(location.lon, location.lat, safeHeight),
        orientation: {
          heading: Cesium.Math.toRadians(location.heading || 0),
          pitch: Cesium.Math.toRadians(location.pitch || -45),
          roll: location.roll || 0.0,
        },
        duration: 2.0,
        complete: function() {
          console.log(`飞行完成: ${location.name}`);
          
          // 飞行完成后，确保相机不会穿模
          const finalPosition = viewer.camera.position;
          const finalDistance = Cesium.Cartesian3.magnitude(finalPosition);
          if (finalDistance < MIN_DISTANCE) {
            // 如果距离太小，调整到安全距离
            const direction = Cesium.Cartesian3.normalize(finalPosition, new Cesium.Cartesian3());
            const safePosition = Cesium.Cartesian3.multiplyByScalar(direction, MIN_DISTANCE, new Cesium.Cartesian3());
            viewer.camera.position = safePosition;
            viewer.camera.lookAt(Cesium.Cartesian3.ZERO, new Cesium.HeadingPitchRange(0, 0, MIN_DISTANCE));
          }
          
          // 立即更新缩放比例
          if (window.updateZoomLevel) {
            window.updateZoomLevel();
          }
          
          // 延迟恢复自动调整，避免立即被调整回默认位置
          setTimeout(() => {
            if (cesiumManager && cesiumManager.setFlyingTo) {
              cesiumManager.setFlyingTo(false);
              console.log("已设置 isFlyingTo = false");
            }
            // 恢复后再次更新缩放比例
            if (window.updateZoomLevel) {
              window.updateZoomLevel();
            }
          }, 1000); // 延迟1秒，确保 flyTo 完全完成
        }
      });
    } catch (error) {
      console.error("flyTo 失败:", error);
      if (window.toast) {
        window.toast.error("飞行到位置失败: " + (error.message || "未知错误"), 3000);
      }
      // 如果 flyTo 失败，至少尝试直接设置相机位置
      try {
        viewer.camera.setView({
          destination: Cesium.Cartesian3.fromDegrees(location.lon, location.lat, safeHeight),
          orientation: {
            heading: Cesium.Math.toRadians(location.heading || 0),
            pitch: Cesium.Math.toRadians(location.pitch || -45),
            roll: location.roll || 0.0,
          }
        });
        console.log(`使用 setView 切换到: ${location.name}`);
      } catch (setViewError) {
        console.error("setView 也失败:", setViewError);
      }
    }
    
    // 在飞行过程中也持续更新缩放比例（更频繁更新，确保实时显示）
    const updateInterval = setInterval(() => {
      if (window.updateZoomLevel) {
        window.updateZoomLevel();
      }
    }, 50); // 每50ms更新一次，确保实时显示
    
    // 飞行完成后清除定时器
    setTimeout(() => {
      clearInterval(updateInterval);
      // 最后再更新一次，确保显示正确
      if (window.updateZoomLevel) {
        window.updateZoomLevel();
      }
    }, 2500); // 略长于飞行时间
    
    console.log(`已切换到: ${location.name}`);
  }

  return {
    loadLocations,
    flyToLocation
  };
}
