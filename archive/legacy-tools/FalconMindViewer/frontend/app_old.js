const { createApp, ref, reactive, onMounted, onUnmounted } = Vue;

// Cesium 全局配置（使用配置服务）
if (window.CONFIG && window.CONFIG.CESIUM_BASE_URL) {
  window.CESIUM_BASE_URL = window.CONFIG.CESIUM_BASE_URL;
} else {
  window.CESIUM_BASE_URL = "./libs/cesium/Build/Cesium/";
}

// UAV 颜色配置（使用配置服务）
const UAV_COLORS = window.CONFIG?.UAV_COLORS || [
  Cesium?.Color?.CYAN || '#00ffff',
  Cesium?.Color?.YELLOW || '#ffff00',
  Cesium?.Color?.LIME || '#00ff00',
  Cesium?.Color?.MAGENTA || '#ff00ff',
  Cesium?.Color?.ORANGE || '#ffa500',
];

createApp({
  setup() {
    // 响应式数据
    const uavStates = reactive({});
    const selectedUavId = ref(null);
    const missions = reactive({});
    const wsStatus = ref("connecting");
    const zoomLevel = ref("1.0x"); // 缩放比例显示
    const locations = ref([]); // 位置列表
    const selectedLocationId = ref(null); // 当前选择的位置ID
    const defaultLocationId = ref(null); // 默认位置ID
    
    // Cesium 相关
    let viewer = null;
    const uavEntities = {};
    const searchAreaEntities = {}; // 搜索区域实体：mission_id -> entity
    const detectionEntities = {}; // 检测结果实体：detection_id -> entity
    const coverageHeatmapEntities = {}; // 搜索覆盖热力图实体：mission_id -> entity
    const searchPathEntities = {}; // 搜索路径实体：mission_id -> {polyline, waypoints}
    const trajectoryHistory = {}; // 历史轨迹：uav_id -> [{position, timestamp}]
    const playbackState = reactive({
      isPlaying: false,
      currentTime: 0,
      playbackSpeed: 1.0,
      startTime: null,
      endTime: null
    });
    let firstTelemetryReceived = false;
    let wsService = null; // 使用WebSocketService
    let missionRefreshInterval = null;
    let cameraAdjustmentFrame = null; // 用于 requestAnimationFrame
    let memoryManager = null; // 内存管理器
    let entityBatcher = null; // 实体批处理器

    // 加载位置配置
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
          height: 500.0,
          heading: 0.0,
          pitch: -45.0,
          roll: 0.0
        }];
        selectedLocationId.value = "changping_park";
        defaultLocationId.value = "changping_park";
      }
    }
    
    // 切换到指定位置
    function flyToLocation(locationId) {
      if (!viewer) return;
      
      const location = locations.value.find(loc => loc.id === locationId);
      if (!location) {
        console.error("位置不存在:", locationId);
        return;
      }
      
      selectedLocationId.value = locationId;
      // 保存到localStorage
      localStorage.setItem("viewer_selected_location", locationId);
      
      viewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(location.lon, location.lat, location.height),
        orientation: {
          heading: Cesium.Math.toRadians(location.heading || 0),
          pitch: Cesium.Math.toRadians(location.pitch || -45),
          roll: location.roll || 0.0,
        },
        duration: 2.0,
        complete: function() {
          // 飞行完成后更新缩放比例
          if (window.updateZoomLevel) {
            window.updateZoomLevel();
          }
        }
      });
      
      // 在飞行过程中也持续更新缩放比例
      const updateInterval = setInterval(() => {
        if (window.updateZoomLevel) {
          window.updateZoomLevel();
        }
      }, 100);
      
      // 飞行完成后清除定时器
      setTimeout(() => {
        clearInterval(updateInterval);
      }, 2500); // 略长于飞行时间
      
      console.log(`已切换到: ${location.name}`);
    }

    // 初始化 Cesium
    function initCesium() {
      // 如果已经初始化过，直接返回
      if (viewer) {
        console.log("Cesium viewer already initialized");
        return;
      }
      
      // 检查容器是否存在
      const container = document.getElementById("cesiumContainer");
      if (!container) {
        console.error("Cesium container 'cesiumContainer' not found!");
        throw new Error("Cesium container element not found");
      }
      
      // 检查Cesium是否已加载
      if (typeof Cesium === 'undefined') {
        console.error("Cesium library not loaded!");
        throw new Error("Cesium library not loaded");
      }
      
      console.log("Initializing Cesium viewer...");
      
      viewer = new Cesium.Viewer("cesiumContainer", {
        animation: false,
        timeline: false,
        geocoder: false,
        homeButton: false,
        sceneModePicker: false,
        baseLayerPicker: true,
        navigationHelpButton: false,
        fullscreenButton: false,
        // 禁用默认的 Cesium Ion 底图
        imageryProvider: false,
      });

      // 移除默认的 Cesium Ion 底图
      viewer.imageryLayers.removeAll();

      // 使用CesiumHelpers配置瓦片加载
      if (window.CesiumHelpers) {
        CesiumHelpers.configureTileLoading(viewer);
        CesiumHelpers.configureRenderPerformance(viewer);
      } else {
        // 回退到原有配置
        viewer.scene.globe.tileCacheSize = 1000;
        viewer.scene.requestRenderMode = false;
        viewer.scene.maximumRenderTimeChange = Infinity;
      }
      
      // 添加 OpenStreetMap 作为默认底图
      // 优先使用本地瓦片（北京市地图已预下载），否则使用在线服务器
      let osmImagery;
      // 本地地图瓦片路径：tiles/{z}/{x}/{y}.png
      const localTilesPath = "./tiles/{z}/{x}/{y}.png";
      
      // 优先使用本地瓦片（北京市地图）
      // 使用本地瓦片（UrlTemplateImageryProvider）
      osmImagery = new Cesium.UrlTemplateImageryProvider({
        url: localTilesPath,
        credit: "© OpenStreetMap contributors (北京市本地地图)",
        maximumLevel: 14,  // 与实际下载的最大缩放级别一致，避免请求不存在的瓦片
        minimumLevel: 0,
        // 优化性能：禁用要素拾取，提高性能
        enablePickFeatures: false,
        // 优化加载：启用重试机制
        hasAlphaChannel: false,
        // 提高清晰度：禁用图像平滑（如果瓦片质量足够高）
        // 注意：如果瓦片本身分辨率不够，禁用平滑可能会让图像看起来更模糊
        // 这里保持默认值（启用平滑），因为OpenStreetMap瓦片质量适中
      });
      console.log("使用本地地图瓦片（北京市）:", localTilesPath);
      
      const osmLayer = viewer.imageryLayers.addImageryProvider(osmImagery);
      
      // 监听瓦片加载错误，静默处理404错误（本地地图未下载的区域）
      // 注意：对于未下载的区域，会显示空白，这是正常的
      // 不显示错误日志，减少控制台噪音，提高性能
      osmLayer.imageryProvider.errorEvent.addEventListener(function(error) {
        // 静默处理404错误（本地地图未下载的区域）
        // 不输出警告，减少控制台噪音，提高性能
        // 如果需要调试，可以取消下面的注释
        // console.warn("本地瓦片加载失败（可能该区域未下载）:", error);
      });
      
      // 优化图层加载
      osmLayer.alpha = 1.0;
      osmLayer.brightness = 1.0;
      osmLayer.contrast = 1.0;
      
      // 预加载相邻瓦片，提高连续性
      viewer.scene.globe.preloadSiblings = true;
      viewer.scene.globe.preloadAncestors = true;
      
      // 优化瓦片加载：增加同时加载的瓦片数量
      viewer.scene.globe.tileCacheSize = 3000; // 进一步增加缓存大小，提高流畅度
      
      // 优化网络请求：增加并发连接数（本地地图不需要太多并发）
      // 注意：需要等待globe完全初始化后再设置，否则会报错
      setTimeout(() => {
        try {
          if (viewer && viewer.scene && viewer.scene.globe && viewer.scene.globe._surface && viewer.scene.globe._surface._tileProvider && viewer.scene.globe._surface._tileProvider._requestScheduler) {
            viewer.scene.globe._surface._tileProvider._requestScheduler.maximumRequests = 50; // 本地地图可以更多并发
            // 优化请求优先级
            viewer.scene.globe._surface._tileProvider._requestScheduler.priorityFunction = function(request) {
              // 优先加载视野中心的瓦片
              return request.priority;
            };
            console.log("✅ 瓦片请求调度器配置成功");
          } else {
            console.warn("⚠️ 瓦片请求调度器未就绪，跳过配置");
          }
        } catch (e) {
          console.warn("⚠️ 配置瓦片请求调度器失败（可能已初始化）:", e);
        }
      }, 500); // 延迟500ms，确保globe完全初始化
      
      // 优化渲染：确保瓦片加载后立即显示
      viewer.scene.globe.enableLighting = false; // 禁用光照，提高性能
      viewer.scene.globe.dynamicAtmosphereLighting = false; // 禁用动态大气光照
      viewer.scene.globe.showWaterEffect = false; // 禁用水面效果，提高性能
      viewer.scene.globe.showGroundAtmosphere = false; // 禁用地面大气效果，提高性能
      
      // 优化瓦片加载优先级
      viewer.scene.globe.tileLoadProgressEvent.addEventListener(function(remaining) {
        // 可以在这里添加加载进度显示
      });
      
      // 优化渲染性能：使用更高效的渲染模式
      viewer.scene.requestRenderMode = false; // 禁用按需渲染，确保连续渲染
      viewer.scene.maximumRenderTimeChange = Infinity; // 不限制渲染时间变化
      
      // 提高渲染质量：禁用FXAA（如果性能允许，可以启用更好的抗锯齿）
      // viewer.scene.postProcessStages.fxaa.enabled = false; // 禁用FXAA可以提高清晰度，但可能有锯齿
      
      // 优化纹理质量：确保使用高质量纹理
      viewer.scene.globe._surface.tileProvider._imageryProvider._tilingScheme = viewer.scene.globe._surface.tileProvider._imageryProvider._tilingScheme;
      
      // 优化瓦片加载策略：提前加载
      viewer.scene.globe.baseColor = Cesium.Color.WHITE; // 设置基础颜色，避免空白
      
      // 优化相机移动时的瓦片加载
      viewer.camera.moveEnd.addEventListener(function() {
        // 相机移动结束后，强制渲染一次，确保新瓦片显示
        viewer.scene.requestRender();
      });
      
      // 强制连续渲染，确保地图瓦片及时加载和显示
      viewer.clock.shouldAnimate = false;
      
      // 优化帧率：限制最大帧率，减少CPU占用，提高流畅度
      viewer.targetFrameRate = 60; // 目标帧率60fps
      
      // 优化渲染：使用更高效的渲染循环
      viewer.useDefaultRenderLoop = true;
      
      // 提高清晰度和流畅度：优化相机变化时的渲染
      viewer.camera.changed.addEventListener(function() {
        // 相机变化时强制渲染，确保地图及时更新
        viewer.scene.requestRender();
      });
      
      // 进一步优化瓦片缓存：提高流畅度
      viewer.scene.globe.tileCacheSize = 5000; // 增加缓存到5000，提高流畅度

      // 设置初始相机位置（使用配置的默认位置）
      // 等待Cesium完全初始化后再设置相机位置
      // 使用更可靠的初始化检查
      let initAttempts = 0;
      const maxAttempts = 20; // 最多尝试20次（2秒）
      
      function tryInitCamera() {
        if (!viewer || !viewer.scene || !viewer.scene.globe) {
          initAttempts++;
          if (initAttempts < maxAttempts) {
            setTimeout(tryInitCamera, 100);
          } else {
            console.warn("Cesium初始化超时，使用默认位置");
            // 即使超时也尝试设置位置
            setInitialCameraPosition();
          }
          return;
        }
        
        // Cesium已初始化，设置相机位置
        setInitialCameraPosition();
      }
      
      function setInitialCameraPosition() {
        const locationId = selectedLocationId.value || defaultLocationId.value;
        if (locationId) {
          const location = locations.value.find(loc => loc.id === locationId);
          if (location) {
            // 使用flyTo而不是setView，提供平滑的动画效果
            viewer.camera.flyTo({
              destination: Cesium.Cartesian3.fromDegrees(location.lon, location.lat, location.height),
              orientation: {
                heading: Cesium.Math.toRadians(location.heading || 0),
                pitch: Cesium.Math.toRadians(location.pitch || -45),
                roll: location.roll || 0.0,
              },
              duration: 2.0, // 2秒动画
              complete: function() {
                // 飞行完成后更新缩放比例
                if (window.updateZoomLevel) {
                  window.updateZoomLevel();
                }
                console.log(`✅ 相机已定位到: ${location.name} (${location.lat}°N, ${location.lon}°E)`);
              }
            });
            console.log(`🔄 开始定位到: ${location.name} (${location.lat}°N, ${location.lon}°E)`);
          }
        } else {
          // 如果没有配置，使用默认的昌平公园位置
          viewer.camera.flyTo({
            destination: Cesium.Cartesian3.fromDegrees(116.2317, 40.2265, 500.0),
            orientation: {
              heading: Cesium.Math.toRadians(0),
              pitch: Cesium.Math.toRadians(-45),
              roll: 0.0,
            },
            duration: 2.0,
            complete: function() {
              // 飞行完成后更新缩放比例
              if (window.updateZoomLevel) {
                window.updateZoomLevel();
              }
              console.log("✅ 相机已定位到默认位置（昌平公园）");
            }
          });
          console.log("🔄 开始定位到默认位置（昌平公园）");
        }
      }
      
      // 延迟启动，确保Cesium完全初始化
      setTimeout(tryInitCamera, 300);
      
      // 确保地球可见
      viewer.scene.globe.show = true;
      
      // 添加错误处理
      viewer.scene.globe.tileLoadErrorEvent.addEventListener(function(error) {
        console.warn("Tile load error:", error);
      });
      
      // 监听场景渲染错误
      viewer.scene.renderError.addEventListener(function(scene, error) {
        console.error("Scene render error:", error);
      });
      
      console.log("Cesium viewer initialized successfully");

      // 相机控制 - 确保地球始终在显示区中央
      // 禁用平移，只允许旋转和缩放（围绕地球中心）
      viewer.scene.screenSpaceCameraController.enableRotate = true;
      viewer.scene.screenSpaceCameraController.enableTranslate = false; // 禁用平移
      viewer.scene.screenSpaceCameraController.enableZoom = true;
      viewer.scene.screenSpaceCameraController.enableTilt = true;
      viewer.scene.screenSpaceCameraController.enableLook = true;
      // 增加最小缩放距离，防止相机进入地球（地球半径约 6371000 米）
      // 最小距离设为 100 米，避免穿模
      viewer.scene.screenSpaceCameraController.minimumZoomDistance = 100.0;
      viewer.scene.screenSpaceCameraController.maximumZoomDistance = 20000000.0;
      
      // 禁用相机惯性
      viewer.scene.screenSpaceCameraController.inertiaSpin = 0.0;
      viewer.scene.screenSpaceCameraController.inertiaTranslate = 0.0;
      viewer.scene.screenSpaceCameraController.inertiaZoom = 0.0;

      // 强制相机始终看向地球中心（原点）
      // 使用 requestAnimationFrame 优化性能，确保拖动流畅
      const earthCenter = Cesium.Cartesian3.ZERO;
      let isAdjusting = false;
      let lastAdjustmentTime = 0;
      
      // 更新缩放比例显示（使用CesiumHelpers）
      function updateZoomLevel() {
        if (!viewer) return;
        try {
          if (window.CesiumHelpers) {
            const zoomInfo = CesiumHelpers.calculateZoomLevel(viewer);
            zoomLevel.value = zoomInfo.display;
          } else {
            // 回退到原有计算
            const cameraPosition = viewer.camera.position;
            const earthRadius = 6371000;
            const distanceToCenter = Cesium.Cartesian3.magnitude(cameraPosition);
            const distanceToSurface = Math.max(0, distanceToCenter - earthRadius);
            const referenceHeight = 1000;
            const zoomRatio = referenceHeight / Math.max(distanceToSurface, 1);
            const heightKm = (distanceToSurface / 1000).toFixed(1);
            zoomLevel.value = `${zoomRatio.toFixed(2)}x (${heightKm} km)`;
          }
        } catch (e) {
          console.error("Failed to update zoom level", e);
        }
      }
      
      // 将 updateZoomLevel 函数暴露到全局作用域，以便 flyToLocation 可以访问
      window.updateZoomLevel = updateZoomLevel;
      
      // 将 updateZoomLevel 函数暴露到全局作用域，以便 flyToLocation 可以访问
      window.updateZoomLevel = updateZoomLevel;
      
      // 使用 requestAnimationFrame 优化相机调整，确保拖动流畅
      const cameraAdjustThrottle = window.CONFIG?.CAMERA_ADJUST?.throttle || 100;
      
      function adjustCameraToCenter() {
        if (isAdjusting) return;
        
        const now = performance.now();
        // 限制调整频率（使用配置）
        const throttleDelay = cameraAdjustThrottle;
        if (now - lastAdjustmentTime < throttleDelay) {
          if (cameraAdjustmentFrame === null) {
            cameraAdjustmentFrame = requestAnimationFrame(adjustCameraToCenter);
          }
          return;
        }
        lastAdjustmentTime = now;
        
        // 计算地球中心在屏幕上的投影
        const screenPosition = viewer.scene.cartesianToCanvasCoordinates(earthCenter);
        if (!screenPosition) {
          // 如果地球中心不在视野内，强制调整相机
          isAdjusting = true;
          let currentHeight = Cesium.Cartesian3.magnitude(viewer.camera.position);
          // 确保最小高度，防止进入地球
          const minHeight = 6371000 + 100; // 地球半径 + 100米
          currentHeight = Math.max(currentHeight, minHeight);
          viewer.camera.lookAt(
            Cesium.Cartesian3.fromDegrees(0, 0, currentHeight),
            new Cesium.HeadingPitchRange(viewer.camera.heading, viewer.camera.pitch, currentHeight)
          );
          setTimeout(() => { isAdjusting = false; }, 33); // 从50ms改为33ms，与调整频率一致
          cameraAdjustmentFrame = null;
          return;
        }
        
        const canvas = viewer.scene.canvas;
        const screenCenter = new Cesium.Cartesian2(canvas.width / 2, canvas.height / 2);
        
        // 计算偏移量
        const offsetX = screenPosition.x - screenCenter.x;
        const offsetY = screenPosition.y - screenCenter.y;
        const threshold = 2; // 2像素阈值，减少频繁调整
        
        // 如果地球中心偏离屏幕中心，调整相机
        if (Math.abs(offsetX) > threshold || Math.abs(offsetY) > threshold) {
          isAdjusting = true;
          
          // 获取当前相机状态
          const currentPosition = viewer.camera.position.clone();
          const currentHeading = viewer.camera.heading;
          const currentPitch = viewer.camera.pitch;
          let distance = Cesium.Cartesian3.magnitude(currentPosition);
          
          // 确保最小距离，防止相机进入地球
          const minDistance = 6371000 + 100; // 地球半径 + 100米
          if (distance < minDistance) {
            distance = minDistance;
            // 调整相机位置到安全距离
            const direction = Cesium.Cartesian3.normalize(currentPosition, new Cesium.Cartesian3());
            const safePosition = Cesium.Cartesian3.multiplyByScalar(direction, distance, new Cesium.Cartesian3());
            viewer.camera.position = safePosition;
            cameraAdjustmentFrame = null;
            return;
          }
          
          // 使用相机的右向量和上向量来调整位置
          const cameraDirection = viewer.camera.direction;
          const cameraUp = viewer.camera.up;
          const cameraRight = Cesium.Cartesian3.cross(cameraDirection, cameraUp, new Cesium.Cartesian3());
          Cesium.Cartesian3.normalize(cameraRight, cameraRight);
          
          // 计算需要调整的世界坐标偏移
          const fov = viewer.camera.frustum.fov || Cesium.Math.toRadians(60);
          const pixelToWorldScale = (distance * Math.tan(fov / 2)) / (canvas.height / 2);
          
          // 使用较小的调整系数，使调整更平滑
          const adjustmentFactor = 0.3;
          const worldOffsetX = -offsetX * pixelToWorldScale * adjustmentFactor;
          const worldOffsetY = offsetY * pixelToWorldScale * adjustmentFactor;
          
          // 计算新位置
          const rightAdjustment = Cesium.Cartesian3.multiplyByScalar(cameraRight, worldOffsetX, new Cesium.Cartesian3());
          const upAdjustment = Cesium.Cartesian3.multiplyByScalar(cameraUp, worldOffsetY, new Cesium.Cartesian3());
          const totalAdjustment = Cesium.Cartesian3.add(rightAdjustment, upAdjustment, new Cesium.Cartesian3());
          const newPosition = Cesium.Cartesian3.add(currentPosition, totalAdjustment, new Cesium.Cartesian3());
          
          // 使用 lookAt 确保地球中心在视野中心
          viewer.camera.lookAt(
            newPosition,
            new Cesium.HeadingPitchRange(currentHeading, currentPitch, distance)
          );
          
          updateZoomLevel();
          
          setTimeout(() => { isAdjusting = false; }, 33); // 从16ms改为33ms，与调整频率一致
        } else {
          updateZoomLevel();
        }
        
        cameraAdjustmentFrame = null;
      }
      
      // 延迟启动相机调整，避免覆盖初始位置
      let cameraAdjustmentEnabled = false;
      setTimeout(() => {
        cameraAdjustmentEnabled = true;
      }, 500); // 延迟 500ms 后再启用相机调整
      
      // 监听相机变化，使用 requestAnimationFrame 优化
      // 优化：使用节流，减少事件处理频率
      let lastChangedTime = 0;
      viewer.camera.changed.addEventListener(function() {
        const now = Date.now();
        // 限制更新频率到约30fps（33ms）
        if (now - lastChangedTime < 33) {
          return;
        }
        lastChangedTime = now;
        
        // 始终更新缩放比例（无论是否调整相机）
        updateZoomLevel();
        
        // 只有在启用后才调整相机位置以保持地球居中
        if (cameraAdjustmentEnabled && cameraAdjustmentFrame === null) {
          cameraAdjustmentFrame = requestAnimationFrame(adjustCameraToCenter);
        }
      });
      
      // 监听相机移动事件（包括鼠标滚轮缩放）
      viewer.camera.moveEnd.addEventListener(function() {
        updateZoomLevel();
      });
      
      // 监听鼠标滚轮事件，实时更新缩放比例
      const canvas = viewer.canvas;
      canvas.addEventListener('wheel', function() {
        // 使用 requestAnimationFrame 确保在渲染后更新
        requestAnimationFrame(updateZoomLevel);
      }, { passive: true });
      
      // 初始化缩放比例
      updateZoomLevel();
      
      // 定期更新缩放比例（作为备用，确保显示更新）
      // 使用更频繁的更新间隔，确保实时性
      setInterval(updateZoomLevel, 50); // 从100ms改为50ms，提高更新频率
    }

    // 创建或获取 UAV 实体
    function getOrCreateUavEntity(uavId) {
      if (!uavEntities[uavId]) {
        const colorIndex = Object.keys(uavEntities).length % UAV_COLORS.length;
        uavEntities[uavId] = viewer.entities.add({
          id: uavId,
          name: uavId,
          position: Cesium.Cartesian3.fromDegrees(116.2317, 40.2265, 100.0), // 北京市昌平区昌平公园
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

    // 选择 UAV
    function selectUav(uavId) {
      selectedUavId.value = uavId;
    }

    // 获取选中的 UAV 信息
    const selectedUavInfo = Vue.computed(() => {
      if (!selectedUavId.value || !uavStates[selectedUavId.value]) {
        return "Select a UAV to view details";
      }

      const t = uavStates[selectedUavId.value];
      const pos = t.position || {};
      const att = t.attitude || {};
      const vel = t.velocity || {};
      const bat = t.battery || {};
      const gps = t.gps || {};

      const lines = [];
      lines.push(`UAV: ${selectedUavId.value}`);
      lines.push(`Lat/Lon/Alt: ${pos.lat?.toFixed(6) ?? "-"}, ${pos.lon?.toFixed(6) ?? "-"}, ${pos.alt?.toFixed(1) ?? "-"} m`);
      lines.push(`Attitude (r/p/y): ${att.roll?.toFixed(2) ?? "-"}, ${att.pitch?.toFixed(2) ?? "-"}, ${att.yaw?.toFixed(2) ?? "-"} rad`);
      lines.push(`Velocity: ${vel.vx?.toFixed(2) ?? "-"}, ${vel.vy?.toFixed(2) ?? "-"}, ${vel.vz?.toFixed(2) ?? "-"} m/s`);
      lines.push(`Battery: ${bat.percent ?? "-"} %, ${bat.voltage_mv ?? "-"} mV`);
      lines.push(`GPS: fix=${gps.fix_type ?? "-"}, sats=${gps.num_sat ?? "-"}`);
      lines.push(`Link: ${t.link_quality ?? "-"} / Mode: ${t.flight_mode || "-"}`);

      return lines.join("\n");
    });

    // 获取 UAV 列表
    const uavList = Vue.computed(() => {
      return Object.keys(uavStates);
    });

    // 获取任务列表
    const missionList = Vue.computed(() => {
      return Object.values(missions);
    });

    // 更新 UAV 遥测
    function updateUavTelemetry(msg) {
      const t = msg.data;
      const uavId = t.uav_id || t.uavId || "unknown";
      
      uavStates[uavId] = t;
      
      // 使用批处理更新实体（如果可用）
      if (entityBatcher && viewer) {
        entityBatcher.queueUpdate(uavId, () => {
          updateUavEntity(uavId, t);
        });
      } else {
        // 回退到直接更新
        updateUavEntity(uavId, t);
      }
    }
    
    // 更新UAV实体的辅助函数
    function updateUavEntity(uavId, t) {

      // 更新实体位置
      if (t.position && t.position.lat !== undefined && t.position.lon !== undefined) {
        const entity = getOrCreateUavEntity(uavId);
        const alt = t.position.alt || 0;
        entity.position = Cesium.Cartesian3.fromDegrees(
          t.position.lon,
          t.position.lat,
          alt
        );
        
        // 记录历史轨迹（使用配置限制）
        if (!trajectoryHistory[uavId]) {
          trajectoryHistory[uavId] = [];
        }
        const timestamp = Date.now();
        trajectoryHistory[uavId].push({
          position: { lat: t.position.lat, lon: t.position.lon, alt: alt },
          timestamp: timestamp
        });
        
        // 只保留配置的时间范围内的数据
        const retentionMs = (window.CONFIG?.TRAJECTORY_RETENTION_HOURS || 1) * 3600000;
        const cutoffTime = timestamp - retentionMs;
        trajectoryHistory[uavId] = trajectoryHistory[uavId].filter(
          point => point.timestamp > cutoffTime
        );
        
        // 限制轨迹点数（使用CesiumHelpers）
        if (window.CesiumHelpers) {
          const maxPoints = window.CONFIG?.MAX_TRAJECTORY_POINTS || 10000;
          const decimation = window.CONFIG?.TRAJECTORY_DECIMATION || 5;
          trajectoryHistory[uavId] = CesiumHelpers.limitTrajectoryPoints(
            trajectoryHistory[uavId],
            maxPoints,
            decimation
          );
        }
        
        // 更新轨迹线
        updateTrajectoryLine(uavId);

        // 首次收到 Telemetry 时，调整相机
        if (!firstTelemetryReceived) {
          firstTelemetryReceived = true;
          // 相机高度 = UAV 高度 + 200 米（降低高度，更接近地面）
          const cameraHeight = Math.max(alt + 200, 100); // 至少 100 米高度
          viewer.camera.flyTo({
            destination: Cesium.Cartesian3.fromDegrees(
              t.position.lon,
              t.position.lat,
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

    // 更新任务列表（使用API服务）
    async function updateMissionList() {
      try {
        const data = await api.getMissions();
        
        // 清空并重新填充
        Object.keys(missions).forEach(key => delete missions[key]);
        if (data.missions) {
          data.missions.forEach(mission => {
            missions[mission.mission_id] = mission;
            // 如果任务包含搜索区域信息，显示搜索区域
            if (mission.payload && mission.payload.search_area) {
              updateSearchAreaForMission(mission.mission_id, mission.payload.search_area);
            }
            // 如果任务包含航点信息，显示搜索路径
            if (mission.payload && mission.payload.waypoints) {
              updateSearchPath(mission.mission_id, mission.payload.waypoints);
            }
          });
        }
      } catch (e) {
        console.error("Failed to fetch missions", e);
      }
    }

    // 更新搜索区域可视化
    function updateSearchAreaForMission(missionId, searchAreaData = null) {
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

    // 更新搜索路径可视化（航点连线）
    function updateSearchPath(missionId, waypointsData = null) {
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

    // 更新搜索区域（从 WebSocket 消息）
    function updateSearchArea(data) {
      if (!data || !data.mission_id) return;
      updateSearchAreaForMission(data.mission_id, data);
    }

    // 更新检测结果标记
    function updateDetection(data) {
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

    // 更新轨迹线
    function updateTrajectoryLine(uavId) {
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

    // 更新搜索覆盖热力图
    function updateCoverageHeatmap(missionId, coverageData) {
      if (!viewer) return;
      
      // 移除旧的热力图
      if (coverageHeatmapEntities[missionId]) {
        viewer.entities.remove(coverageHeatmapEntities[missionId]);
        delete coverageHeatmapEntities[missionId];
      }
      
      if (!coverageData || !coverageData.coverage_points || coverageData.coverage_points.length === 0) {
        return;
      }
      
      // 创建热力图：使用多个半透明圆来表示覆盖密度
      const coveragePoints = coverageData.coverage_points;
      const maxCoverage = Math.max(...coveragePoints.map(p => p.coverage || 0));
      
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
            semiMajorAxis: point.radius || 50, // 覆盖半径（米）
            semiMinorAxis: point.radius || 50,
            material: color,
            outline: true,
            outlineColor: color,
            height: point.alt || 0,
          },
        });
        
        if (!coverageHeatmapEntities[missionId]) {
          coverageHeatmapEntities[missionId] = [];
        }
        coverageHeatmapEntities[missionId].push(entity);
      });
    }

    // 轨迹回放控制
    function startPlayback(uavId) {
      if (!trajectoryHistory[uavId] || trajectoryHistory[uavId].length < 2) {
        if (window.toast) {
          window.toast.warning("没有可用的轨迹数据用于回放");
        } else {
          alert("No trajectory data available for playback");
        }
        return;
      }
      
      const trajectory = trajectoryHistory[uavId];
      playbackState.isPlaying = true;
      playbackState.startTime = trajectory[0].timestamp;
      playbackState.endTime = trajectory[trajectory.length - 1].timestamp;
      playbackState.currentTime = playbackState.startTime;
      
      // 开始回放动画
      playbackAnimation(uavId);
    }

    function stopPlayback() {
      playbackState.isPlaying = false;
    }

    function playbackAnimation(uavId) {
      if (!playbackState.isPlaying || !trajectoryHistory[uavId]) {
        return;
      }
      
      const trajectory = trajectoryHistory[uavId];
      const currentTime = playbackState.currentTime;
      
      // 找到当前时间对应的位置
      let currentIndex = 0;
      for (let i = 0; i < trajectory.length - 1; i++) {
        if (trajectory[i].timestamp <= currentTime && trajectory[i + 1].timestamp >= currentTime) {
          currentIndex = i;
          break;
        }
      }
      
      if (currentIndex < trajectory.length) {
        const point = trajectory[currentIndex];
        const entity = uavEntities[uavId];
        if (entity) {
          entity.position = Cesium.Cartesian3.fromDegrees(
            point.position.lon,
            point.position.lat,
            point.position.alt
          );
        }
      }
      
      // 更新当前时间
      playbackState.currentTime += 1000 * playbackState.playbackSpeed; // 每秒前进
      
      if (playbackState.currentTime >= playbackState.endTime) {
        playbackState.currentTime = playbackState.endTime;
        stopPlayback();
      } else {
        requestAnimationFrame(() => playbackAnimation(uavId));
      }
    }

    // 处理搜索进度消息
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

    // 创建测试任务（使用API服务）
    async function createTestMission() {
      try {
        await api.createMission({
          name: `Test Mission ${new Date().toLocaleTimeString()}`,
          description: "Test mission created from Viewer",
          mission_type: "SINGLE_UAV",
          uav_list: uavList.value.slice(0, 1),
          payload: {},
        });
        await updateMissionList();
      } catch (e) {
        console.error("Failed to create mission", e);
        if (window.toast) {
          window.toast.error("创建任务失败: " + e.message);
        } else {
          alert("Failed to create mission: " + e.message);
        }
      }
    }

    // 任务操作（使用API服务）
    async function dispatchMission(missionId) {
      try {
        await api.dispatchMission(missionId);
        await updateMissionList();
      } catch (e) {
        console.error("Failed to dispatch mission", e);
        if (window.toast) {
          window.toast.error("下发任务失败: " + e.message);
        } else {
          alert("Failed to dispatch mission: " + e.message);
        }
      }
    }

    async function pauseMission(missionId) {
      try {
        await api.pauseMission(missionId);
        await updateMissionList();
      } catch (e) {
        console.error("Failed to pause mission", e);
        if (window.toast) {
          window.toast.error("暂停任务失败: " + e.message);
        } else {
          alert("Failed to pause mission: " + e.message);
        }
      }
    }

    async function resumeMission(missionId) {
      try {
        await api.resumeMission(missionId);
        await updateMissionList();
      } catch (e) {
        console.error("Failed to resume mission", e);
        if (window.toast) {
          window.toast.error("恢复任务失败: " + e.message);
        } else {
          alert("Failed to resume mission: " + e.message);
        }
      }
    }

    async function cancelMission(missionId) {
      try {
        await api.cancelMission(missionId);
        await updateMissionList();
      } catch (e) {
        console.error("Failed to cancel mission", e);
        if (window.toast) {
          window.toast.error("取消任务失败: " + e.message);
        } else {
          alert("Failed to cancel mission: " + e.message);
        }
      }
    }

    async function deleteMission(missionId) {
      if (!confirm("Are you sure you want to delete this mission?")) {
        return;
      }
      try {
        await api.deleteMission(missionId);
        await updateMissionList();
      } catch (e) {
        console.error("Failed to delete mission", e);
        if (window.toast) {
          window.toast.error("删除任务失败: " + (e.message || "未知错误"));
        } else {
          alert("Failed to delete mission: " + (e.message || "Unknown error"));
        }
      }
    }

    // WebSocket 连接（使用优化后的WebSocketService）
    function connectWs() {
      const wsUrl = window.CONFIG?.WS_URL || 
        ((location.protocol === "https:" ? "wss://" : "ws://") +
         (location.hostname || "127.0.0.1") + ":9000/ws/telemetry");

      const wsConfig = window.CONFIG?.WS_RECONNECT || {
        maxAttempts: 10,
        initialDelay: 2000,
        maxDelay: 30000,
        heartbeatInterval: 30000,
      };

      // 创建WebSocket服务实例
      wsService = new WebSocketService(wsUrl, wsConfig);
      
      // 监听连接事件
      wsService.on('connected', () => {
        wsStatus.value = "connected";
        console.log("WebSocket连接成功");
        // 使用Toast通知
        if (window.toast) {
          window.toast.success("WebSocket连接成功");
        }
        // 更新性能监控
        if (window.performanceMonitor) {
          window.performanceMonitor.setWebSocketStatus('connected');
        }
      });
      
      // 监听消息事件
      wsService.on('message', (msg) => {
        if (msg.type === "telemetry") {
          updateUavTelemetry(msg);
        } else if (msg.type === "mission_event") {
          updateMissionList();
          // 如果任务包含搜索区域，显示搜索区域
          if (msg.data && msg.data.mission_id) {
            updateSearchAreaForMission(msg.data.mission_id);
          }
        } else if (msg.type === "search_area") {
          // 搜索区域更新
          updateSearchArea(msg.data);
        } else if (msg.type === "search_path" || msg.type === "waypoints") {
          // 搜索路径/航点更新
          if (msg.data && msg.data.mission_id) {
            updateSearchPath(msg.data.mission_id, msg.data.waypoints || msg.data);
          }
        } else if (msg.type === "detection") {
          // 检测结果更新
          updateDetection(msg.data);
        } else if (msg.type === "search_progress") {
          // 搜索进度更新
          handleSearchProgress(msg.data);
        }
      });
      
      // 监听断开事件
      wsService.on('disconnected', (data) => {
        wsStatus.value = "disconnected";
        console.log("WebSocket连接断开", data);
      });
      
      // 监听重连事件
      wsService.on('reconnecting', (data) => {
        wsStatus.value = `reconnecting... (${data.attempt}/${data.maxAttempts})`;
        console.log("WebSocket重连中", data);
      });
      
      // 监听最大重连次数达到事件
      wsService.on('max_reconnect_reached', () => {
        wsStatus.value = "connection failed";
        console.error("WebSocket达到最大重连次数，连接失败");
        // 使用Toast通知替代alert
        if (window.toast) {
          window.toast.error("WebSocket连接失败，已达到最大重连次数。请检查网络连接或刷新页面。", 8000);
        } else {
          alert("WebSocket连接失败，已达到最大重连次数。请检查网络连接或刷新页面。");
        }
        // 更新性能监控
        if (window.performanceMonitor) {
          window.performanceMonitor.setWebSocketStatus('failed');
        }
      });
      
      // 监听错误事件
      wsService.on('error', (error) => {
        console.error("WebSocket错误", error);
        wsStatus.value = "error";
        // 更新性能监控
        if (window.performanceMonitor) {
          window.performanceMonitor.setWebSocketStatus('error');
        }
      });
      
      // 监听连接成功事件，更新性能监控
      wsService.on('connected', () => {
        if (window.performanceMonitor) {
          window.performanceMonitor.setWebSocketStatus('connected');
        }
      });
      
      // 开始连接
      wsService.connect();
    }

    // 生命周期
    onMounted(async () => {
      // 先加载位置配置
      await loadLocations();
      
      // 延迟初始化Cesium，确保DOM完全渲染和Cesium库加载完成
      // 增加延迟时间，给Cesium库更多时间加载
      setTimeout(() => {
        // 检查Cesium是否已加载
        if (typeof Cesium === 'undefined') {
          console.warn("Cesium library not loaded yet, retrying...");
          // 如果Cesium未加载，再等待一段时间后重试
          setTimeout(() => {
            try {
              initCesium();
            } catch (e) {
              console.error("Failed to initialize Cesium after retry:", e);
              // 只在确实失败时才显示错误提示
              if (!viewer) {
                const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                if (window.toast) {
                  window.toast.error(errorMsg, 10000);
                } else {
                  alert(errorMsg);
                }
              }
            }
          }, 500);
        } else {
          try {
            initCesium();
            // 延迟检查是否初始化成功，避免过早显示错误
            // 如果初始化成功，viewer会被设置，不会显示错误提示
            setTimeout(() => {
              if (!viewer) {
                console.warn("Cesium viewer not initialized after 2 seconds, checking again...");
                // 再等待一段时间，如果还是失败才显示错误
                setTimeout(() => {
                  if (!viewer) {
                    console.error("Cesium viewer still not initialized after 4 seconds");
                    const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                if (window.toast) {
                  window.toast.error(errorMsg, 10000);
                } else {
                  alert(errorMsg);
                }
                  }
                }, 2000);
              } else {
                console.log("Cesium viewer initialized successfully");
              }
            }, 2000);
          } catch (e) {
            console.error("Failed to initialize Cesium:", e);
            // 延迟显示错误，给初始化更多时间（可能只是临时错误）
            setTimeout(() => {
              if (!viewer) {
                const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                if (window.toast) {
                  window.toast.error(errorMsg, 10000);
                } else {
                  alert(errorMsg);
                }
              }
            }, 2000);
          }
        }
      }, 200); // 增加初始延迟到200ms
      
      connectWs();
      updateMissionList();
      
      // 定期刷新任务列表
      missionRefreshInterval = setInterval(updateMissionList, 5000);
      
      // 注册键盘快捷键
      if (window.keyboardShortcuts) {
        // 注册快捷键（调用工具栏函数）
        window.keyboardShortcuts.register('f', '聚焦选中的UAV', focusSelectedUav);
        window.keyboardShortcuts.register('r', '重置相机到默认位置', resetCamera);
        window.keyboardShortcuts.register('c', '居中显示所有UAV', centerAllUavs);
        window.keyboardShortcuts.register('Escape', '取消选择', clearSelection);
        window.keyboardShortcuts.register(' ', '暂停/继续轨迹回放', togglePlayback);
        window.keyboardShortcuts.register('=', '加快回放速度', speedUpPlayback, { shift: false });
        window.keyboardShortcuts.register('-', '减慢回放速度', speedDownPlayback);
        window.keyboardShortcuts.register('s', '保存当前视图', saveView, { ctrl: true });
        window.keyboardShortcuts.register('r', '恢复保存的视图', restoreView, { ctrl: true });
        
        console.log('键盘快捷键已注册');
      }
      
      // 初始化下拉菜单功能（延迟初始化，确保DOM已渲染）
      setTimeout(() => {
        initDropdownMenus();
      }, 100);
    });
    
    // 初始化下拉菜单
    function initDropdownMenus() {
      if (!window.dropdownManager) {
        console.warn('DropdownManager not available');
        return;
      }
      
      // 导航菜单项
      window.navigationMenuItems = [
        {
          label: '聚焦选中的UAV',
          icon: '🎯',
          shortcut: 'F',
          action: focusSelectedUav
        },
        {
          label: '居中显示所有UAV',
          icon: '📍',
          shortcut: 'C',
          action: centerAllUavs
        },
        {
          label: '重置相机到默认位置',
          icon: '🏠',
          shortcut: 'R',
          action: resetCamera
        },
        'divider',
        {
          label: '取消选择',
          icon: '✕',
          shortcut: 'ESC',
          action: clearSelection
        }
      ];
      
      // 回放菜单项
      window.playbackMenuItems = [
        {
          label: playbackState.isPlaying ? '暂停回放' : '继续回放',
          icon: playbackState.isPlaying ? '⏸' : '▶',
          shortcut: 'Space',
          action: togglePlayback
        },
        'divider',
        {
          label: '加快回放速度',
          icon: '⏩',
          shortcut: '+',
          action: speedUpPlayback
        },
        {
          label: '减慢回放速度',
          icon: '⏪',
          shortcut: '-',
          action: speedDownPlayback
        },
        'divider',
        {
          label: '开始回放',
          icon: '▶',
          action: startPlayback,
          disabled: playbackState.isPlaying
        },
        {
          label: '停止回放',
          icon: '⏹',
          action: stopPlayback,
          disabled: !playbackState.isPlaying
        }
      ];
      
      // 视图菜单项
      window.viewMenuItems = [
        {
          label: '保存当前视图',
          icon: '💾',
          shortcut: 'Ctrl+S',
          action: saveView
        },
        {
          label: '恢复保存的视图',
          icon: '↩',
          shortcut: 'Ctrl+R',
          action: restoreView
        }
      ];
      
      // 工具菜单项
      window.toolsMenuItems = [
        {
          label: '数据查询',
          icon: '📊',
          action: () => {
            if (window.dataQueryPanel) {
              window.dataQueryPanel.open();
            }
          }
        },
        'divider',
        {
          label: '显示快捷键帮助',
          icon: '❓',
          shortcut: 'Shift+?',
          action: showShortcutsHelp
        },
        {
          label: '显示性能监控',
          icon: '📈',
          shortcut: 'Ctrl+Shift+P',
          action: () => {
            if (window.performanceMonitor) {
              window.performanceMonitor.toggle();
            }
          }
        },
        {
          label: '清除所有通知',
          icon: '🗑',
          action: () => {
            if (window.toast) {
              window.toast.clear();
            }
          }
        }
      ];
    }
    
    // 下拉菜单切换函数
    function toggleNavigationMenu(event) {
      if (!window.dropdownManager || !window.navigationMenuItems) {
        console.warn('DropdownManager or navigationMenuItems not available');
        return;
      }
      if (event) {
        event.stopPropagation();
      }
      const dropdown = window.dropdownManager.getDropdown('navigation');
      const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
      if (button) {
        button.classList.toggle('active');
      }
      dropdown.toggle(window.navigationMenuItems, button);
    }
    
    function togglePlaybackMenu(event) {
      if (!window.dropdownManager || !window.playbackMenuItems) {
        console.warn('DropdownManager or playbackMenuItems not available');
        return;
      }
      if (event) {
        event.stopPropagation();
      }
      // 更新菜单项状态
      if (window.playbackMenuItems && window.playbackMenuItems.length > 0) {
        window.playbackMenuItems[0].label = playbackState.isPlaying ? '暂停回放' : '继续回放';
        window.playbackMenuItems[0].icon = playbackState.isPlaying ? '⏸' : '▶';
        if (window.playbackMenuItems.length > 4) {
          window.playbackMenuItems[4].disabled = playbackState.isPlaying;
          window.playbackMenuItems[5].disabled = !playbackState.isPlaying;
        }
      }
      
      const dropdown = window.dropdownManager.getDropdown('playback');
      const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
      if (button) {
        button.classList.toggle('active');
      }
      dropdown.toggle(window.playbackMenuItems, button);
    }
    
    function toggleViewMenu(event) {
      if (!window.dropdownManager || !window.viewMenuItems) {
        console.warn('DropdownManager or viewMenuItems not available');
        return;
      }
      if (event) {
        event.stopPropagation();
      }
      const dropdown = window.dropdownManager.getDropdown('view');
      const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
      if (button) {
        button.classList.toggle('active');
      }
      dropdown.toggle(window.viewMenuItems, button);
    }
    
    function toggleToolsMenu(event) {
      if (!window.dropdownManager || !window.toolsMenuItems) {
        console.warn('DropdownManager or toolsMenuItems not available');
        return;
      }
      if (event) {
        event.stopPropagation();
      }
      const dropdown = window.dropdownManager.getDropdown('tools');
      const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
      if (button) {
        button.classList.toggle('active');
      }
      dropdown.toggle(window.toolsMenuItems, button);
    }

    onUnmounted(() => {
      if (wsService) {
        wsService.disconnect();
      }
      if (missionRefreshInterval) {
        clearInterval(missionRefreshInterval);
      }
      if (memoryManager) {
        memoryManager.stop();
      }
      if (entityBatcher) {
        entityBatcher.clear();
      }
    });

    // 工具栏功能函数（与快捷键功能对应）
    function focusSelectedUav() {
      if (selectedUavId.value && viewer) {
        const uavState = uavStates[selectedUavId.value];
        if (uavState && uavState.position) {
          const pos = uavState.position;
          viewer.camera.flyTo({
            destination: Cesium.Cartesian3.fromDegrees(
              pos.lon,
              pos.lat,
              (pos.alt || 0) + 200
            ),
            duration: 1.5
          });
          if (window.toast) {
            window.toast.info(`聚焦到 ${selectedUavId.value}`);
          }
        } else {
          if (window.toast) {
            window.toast.warning('选中的UAV没有位置信息');
          }
        }
      } else {
        if (window.toast) {
          window.toast.warning('请先选择一个UAV');
        }
      }
    }
    
    function resetCamera() {
      if (viewer && defaultLocationId.value) {
        const location = locations.value.find(l => l.id === defaultLocationId.value);
        if (location) {
          flyToLocation(location.id);
          if (window.toast) {
            window.toast.info('相机已重置到默认位置');
          }
        }
      }
    }
    
    function centerAllUavs() {
      if (viewer && Object.keys(uavStates).length > 0) {
        viewer.zoomTo(viewer.entities);
        if (window.toast) {
          window.toast.info(`居中显示 ${Object.keys(uavStates).length} 个UAV`);
        }
      } else {
        if (window.toast) {
          window.toast.warning('没有可用的UAV');
        }
      }
    }
    
    function clearSelection() {
      selectedUavId.value = null;
      if (window.toast) {
        window.toast.info('已取消选择');
      }
    }
    
    function togglePlayback() {
      if (playbackState.isPlaying) {
        playbackState.isPlaying = false;
        if (window.toast) {
          window.toast.info('回放已暂停');
        }
      } else {
        playbackState.isPlaying = true;
        if (window.toast) {
          window.toast.info('回放已继续');
        }
      }
    }
    
    function speedUpPlayback() {
      playbackState.playbackSpeed = Math.min(playbackState.playbackSpeed * 1.5, 10);
      if (window.toast) {
        window.toast.info(`回放速度: ${playbackState.playbackSpeed.toFixed(1)}x`);
      }
    }
    
    function speedDownPlayback() {
      playbackState.playbackSpeed = Math.max(playbackState.playbackSpeed / 1.5, 0.1);
      if (window.toast) {
        window.toast.info(`回放速度: ${playbackState.playbackSpeed.toFixed(1)}x`);
      }
    }
    
    function saveView() {
      if (viewer) {
        const camera = viewer.camera;
        const position = camera.position;
        const cartographic = Cesium.Cartographic.fromCartesian(position);
        const savedView = {
          longitude: Cesium.Math.toDegrees(cartographic.longitude),
          latitude: Cesium.Math.toDegrees(cartographic.latitude),
          height: cartographic.height,
          heading: camera.heading,
          pitch: camera.pitch,
          roll: camera.roll
        };
        localStorage.setItem('savedView', JSON.stringify(savedView));
        if (window.toast) {
          window.toast.success('视图已保存');
        }
      }
    }
    
    function restoreView() {
      const savedViewStr = localStorage.getItem('savedView');
      if (savedViewStr && viewer) {
        try {
          const savedView = JSON.parse(savedViewStr);
          viewer.camera.flyTo({
            destination: Cesium.Cartesian3.fromDegrees(
              savedView.longitude,
              savedView.latitude,
              savedView.height
            ),
            orientation: {
              heading: savedView.heading,
              pitch: savedView.pitch,
              roll: savedView.roll
            },
            duration: 1.5
          });
          if (window.toast) {
            window.toast.success('视图已恢复');
          }
        } catch (e) {
          if (window.toast) {
            window.toast.error('恢复视图失败');
          }
        }
      } else {
        if (window.toast) {
          window.toast.warning('没有保存的视图');
        }
      }
    }
    
    function showShortcutsHelp() {
      if (window.keyboardShortcuts) {
        window.keyboardShortcuts.toggleHelp();
      }
    }

    return {
      uavStates,
      selectedUavId,
      zoomLevel,
      missions,
      wsStatus,
      uavList,
      missionList,
      selectedUavInfo,
      selectUav,
      createTestMission,
      dispatchMission,
      pauseMission,
      resumeMission,
      cancelMission,
      deleteMission,
      trajectoryHistory,
      playbackState,
      startPlayback,
      stopPlayback,
      locations,
      selectedLocationId,
      flyToLocation,
      // 工具栏功能
      focusSelectedUav,
      resetCamera,
      centerAllUavs,
      clearSelection,
      togglePlayback,
      speedUpPlayback,
      speedDownPlayback,
      saveView,
      restoreView,
      showShortcutsHelp,
      // 下拉菜单控制
      toggleNavigationMenu,
      togglePlaybackMenu,
      toggleViewMenu,
      toggleToolsMenu,
    };
  },
  template: `
    <div id="app">
      <!-- 工具栏 -->
      <div class="toolbar">
        <!-- 导航菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleNavigationMenu($event)"
            title="导航功能"
          >
            <span class="toolbar-dropdown-label">导航</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 回放菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="togglePlaybackMenu($event)"
            title="回放控制"
          >
            <span class="toolbar-dropdown-label">回放</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 视图菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleViewMenu($event)"
            title="视图管理"
          >
            <span class="toolbar-dropdown-label">视图</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 工具菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleToolsMenu($event)"
            title="工具"
          >
            <span class="toolbar-dropdown-label">工具</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 回放状态显示 -->
        <div class="toolbar-status" v-if="playbackState.isPlaying">
          <span class="toolbar-status-label">回放中</span>
          <span class="toolbar-status-value">{{ playbackState.playbackSpeed.toFixed(1) }}x</span>
        </div>
      </div>
      
      <div class="main-content">
        <div id="cesiumContainer" class="cesium-container">
          <!-- 缩放比例显示 -->
          <div class="zoom-indicator">
            Zoom: {{ zoomLevel }}
          </div>
        </div>
        <div class="sidepanel">
        <h1>FalconMindViewer</h1>
        
        <div class="section">
          <h2>位置选择</h2>
          <select 
            v-model="selectedLocationId" 
            @change="flyToLocation(selectedLocationId)"
            class="location-select"
            style="width: 100%; padding: 8px; margin-bottom: 8px; font-size: 14px; background: rgba(159, 180, 255, 0.1); border: 1px solid rgba(159, 180, 255, 0.3); border-radius: 4px; color: #cfd7ff;"
          >
            <option 
              v-for="loc in locations" 
              :key="loc.id" 
              :value="loc.id"
            >
              {{ loc.name }}
            </option>
          </select>
          <div style="font-size: 12px; color: #999; margin-top: 4px;">
            {{ locations.find(l => l.id === selectedLocationId)?.description || "" }}
          </div>
        </div>

        <div class="section">
          <h2>UAVs</h2>
          <div class="uav-list">
            <div
              v-for="uavId in uavList"
              :key="uavId"
              :class="['uav-item', { selected: selectedUavId === uavId }]"
              @click="selectUav(uavId)"
            >
              {{ uavId }}
            </div>
            <div v-if="uavList.length === 0" class="empty-state">
              No UAVs connected
            </div>
          </div>
          <div class="uav-info">{{ selectedUavInfo }}</div>
        </div>

        <div class="section">
          <h2>Missions</h2>
          <div class="mission-list">
            <div
              v-for="mission in missionList"
              :key="mission.mission_id"
              class="mission-item"
            >
              <div class="mission-item-header">
                <span class="mission-name">{{ mission.name }}</span>
                <span :class="['mission-state', mission.state]">{{ mission.state }}</span>
              </div>
              <div style="font-size: 10px; color: #999; margin-bottom: 4px;">
                {{ mission.mission_id }} | Progress: {{ (mission.progress * 100).toFixed(0) }}%
              </div>
              <div class="mission-actions">
                <button
                  v-if="mission.state === 'PENDING'"
                  class="btn btn-primary"
                  @click="dispatchMission(mission.mission_id)"
                >
                  Dispatch
                </button>
                <button
                  v-if="mission.state === 'RUNNING'"
                  class="btn"
                  @click="pauseMission(mission.mission_id)"
                >
                  Pause
                </button>
                <button
                  v-if="mission.state === 'PAUSED'"
                  class="btn btn-primary"
                  @click="resumeMission(mission.mission_id)"
                >
                  Resume
                </button>
                <button
                  v-if="['PENDING', 'RUNNING', 'PAUSED'].includes(mission.state)"
                  class="btn"
                  @click="cancelMission(mission.mission_id)"
                >
                  Cancel
                </button>
                <button
                  v-if="['SUCCEEDED', 'FAILED', 'CANCELLED'].includes(mission.state)"
                  class="btn"
                  style="background: #ff4444; color: white;"
                  @click="deleteMission(mission.mission_id)"
                >
                  Delete
                </button>
              </div>
            </div>
            <div v-if="missionList.length === 0" class="empty-state">
              No missions
            </div>
          </div>
          <button
            class="btn btn-primary"
            @click="createTestMission"
            style="width: 100%; margin-top: 8px;"
          >
            + Create Test Mission
          </button>
        </div>

        <div class="connection-status">
          Backend WS: <span>{{ wsStatus }}</span>
        </div>

        <!-- 轨迹回放控制 -->
        <div class="section playback-control" v-if="selectedUavId && trajectoryHistory[selectedUavId] && trajectoryHistory[selectedUavId].length > 0">
          <h2>轨迹回放</h2>
          <div class="playback-buttons">
            <button
              class="btn btn-primary"
              @click="startPlayback(selectedUavId)"
              :disabled="playbackState.isPlaying"
            >
              ▶ 开始回放
            </button>
            <button
              class="btn"
              @click="stopPlayback"
              :disabled="!playbackState.isPlaying"
            >
              ⏸ 停止
            </button>
          </div>
          <div class="playback-info" v-if="playbackState.isPlaying">
            <div>回放速度: {{ playbackState.playbackSpeed }}x</div>
            <div>时间: {{ new Date(playbackState.currentTime).toLocaleTimeString() }}</div>
          </div>
        </div>
        </div>
      </div>
    </div>
  `,
}).mount("#app");
