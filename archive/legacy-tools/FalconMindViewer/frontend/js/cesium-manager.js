/**
 * Cesium 管理器（重构版）
 * 处理 Cesium 初始化和配置
 * 使用更简洁、可靠的实现
 */
function createCesiumManager(state, locationManager, viewerRef, cameraAdjustmentFrameRef) {
  const { zoomLevel, locations, selectedLocationId, defaultLocationId } = state;
  let cameraAdjustmentEnabled = false;
  let isAdjusting = false;
  let lastAdjustmentTime = 0;
  let isFlyingTo = false; // 标记是否正在 flyTo
  let isUserInteracting = false; // 用户交互标记（提升到函数作用域）
  let isWheeling = false; // 滚轮缩放标记（提升到函数作用域）
  let interactionTimeout = null; // 交互超时标记
  const earthCenter = Cesium.Cartesian3.ZERO;
  const EARTH_RADIUS = 6371000; // 地球半径（米）
  const MIN_DISTANCE = EARTH_RADIUS + 50; // 最小距离：50米高度，允许看到18级瓦片
  const MAX_DISTANCE = EARTH_RADIUS * 2.2; // 最大距离：能看到整个地球

  // UAV 颜色配置
  const UAV_COLORS = window.CONFIG?.UAV_COLORS || [
    Cesium?.Color?.CYAN || '#00ffff',
    Cesium?.Color?.YELLOW || '#ffff00',
    Cesium?.Color?.LIME || '#00ff00',
    Cesium?.Color?.MAGENTA || '#ff00ff',
    Cesium?.Color?.ORANGE || '#ffa500',
  ];

  /**
   * 计算当前相机位置对应的瓦片级别
   */
  function calculateTileLevel(viewer) {
    if (!viewer) return 0;
    
    try {
      const cameraPosition = viewer.camera.position;
      const distanceToCenter = Cesium.Cartesian3.magnitude(cameraPosition);
      const distanceToSurface = Math.max(0, distanceToCenter - EARTH_RADIUS);
      
      // 根据高度估算瓦片级别（OpenStreetMap瓦片级别对应的高度）
      if (distanceToSurface <= 50) return 18;
      if (distanceToSurface <= 100) return 17;
      if (distanceToSurface <= 200) return 16;
      if (distanceToSurface <= 400) return 15;
      if (distanceToSurface <= 800) return 14;
      if (distanceToSurface <= 1600) return 13;
      if (distanceToSurface <= 3200) return 12;
      if (distanceToSurface <= 6400) return 11;
      if (distanceToSurface <= 12800) return 10;
      if (distanceToSurface <= 25600) return 9;
      if (distanceToSurface <= 51200) return 8;
      if (distanceToSurface <= 102400) return 7;
      if (distanceToSurface <= 204800) return 6;
      if (distanceToSurface <= 409600) return 5;
      if (distanceToSurface <= 819200) return 4;
      if (distanceToSurface <= 1638400) return 3;
      if (distanceToSurface <= 3276800) return 2;
      if (distanceToSurface <= 6553600) return 1;
      return 0;
    } catch (e) {
      console.error("Failed to calculate tile level", e);
      return 0;
    }
  }

  /**
   * 更新缩放比例显示（包含地图级别）
   */
  function updateZoomLevel() {
    const viewer = viewerRef.current;
    if (!viewer) {
      console.warn("updateZoomLevel: viewer not ready");
      return;
    }
    try {
      const cameraPosition = viewer.camera.position;
      if (!cameraPosition) {
        console.warn("updateZoomLevel: camera position not available");
        return;
      }
      
      const distanceToCenter = Cesium.Cartesian3.magnitude(cameraPosition);
      const distanceToSurface = Math.max(0, distanceToCenter - EARTH_RADIUS);
      
      // 计算瓦片级别
      const tileLevel = calculateTileLevel(viewer);
      
      // 计算缩放比例
      const referenceHeight = 1000;
      const zoomRatio = referenceHeight / Math.max(distanceToSurface, 1);
      
      // 格式化高度显示
      let heightDisplay;
      if (distanceToSurface < 1000) {
        heightDisplay = `${distanceToSurface.toFixed(0)} m`;
      } else if (distanceToSurface < 1000000) {
        heightDisplay = `${(distanceToSurface / 1000).toFixed(1)} km`;
      } else {
        heightDisplay = `${(distanceToSurface / 1000000).toFixed(2)} Mm`;
      }
      
      // 显示格式：地图级别 | 缩放比例 | 高度
      const newValue = `级别 ${tileLevel} | ${zoomRatio.toFixed(2)}x | ${heightDisplay}`;
      
      // 强制更新，即使值相同也更新（确保Vue响应式更新）
      zoomLevel.value = newValue;
      
      // 调试日志（仅在值变化时输出）
      if (window.DEBUG_ZOOM) {
        console.log("Zoom level updated:", newValue, "distanceToSurface:", distanceToSurface);
      }
    } catch (e) {
      console.error("Failed to update zoom level", e);
    }
  }

  /**
   * 确保相机距离安全（防止穿模）
   * 只在距离小于最小值时才调整，允许正常缩放
   * 注意：不会改变相机朝向，只调整距离
   */
  function ensureSafeDistance() {
    const viewer = viewerRef.current;
    if (!viewer) return;
    
    // 如果用户正在交互，不执行距离检查，避免干扰用户操作
    if (isUserInteracting || isWheeling || isFlyingTo) {
      return;
    }
    
    const position = viewer.camera.position;
    const distance = Cesium.Cartesian3.magnitude(position);
    
    // 只在距离小于最小值时才调整（防止穿模）
    // 允许距离在 MIN_DISTANCE 和 MAX_DISTANCE 之间正常变化
    if (distance < MIN_DISTANCE) {
      // 距离太小，调整到安全距离（保持当前朝向，只调整距离）
      const direction = Cesium.Cartesian3.normalize(position, new Cesium.Cartesian3());
      const safePosition = Cesium.Cartesian3.multiplyByScalar(direction, MIN_DISTANCE, new Cesium.Cartesian3());
      // 只调整位置，不改变朝向（不使用lookAt，避免回到地球中心）
      viewer.camera.position = safePosition;
    }
    
    // 限制最大距离（可选，防止过度缩小）
    if (distance > MAX_DISTANCE) {
      const direction = Cesium.Cartesian3.normalize(position, new Cesium.Cartesian3());
      const safePosition = Cesium.Cartesian3.multiplyByScalar(direction, MAX_DISTANCE, new Cesium.Cartesian3());
      // 只调整位置，不改变朝向（不使用lookAt，避免回到地球中心）
      viewer.camera.position = safePosition;
    }
  }

  /**
   * 以地球中心为目标锁定相机（数字地球标准：Heading/Pitch/Range 围绕 earthCenter）
   */
  function clampRange(range) {
    return Math.max(MIN_DISTANCE, Math.min(MAX_DISTANCE, range));
  }

  /** 缩放时使用：限制的是「相机到地表目标点的距离」（高度），不是到地心的距离 */
  function clampHeightAboveTarget(height) {
    const minHeight = 50; // 最小高度 50 米
    const maxHeight = EARTH_RADIUS * 1.2; // 最大高度，能看到整个地球
    return Math.max(minHeight, Math.min(maxHeight, height));
  }

  function lockCameraOnEarthCenter() {
    const viewer = viewerRef.current;
    // 如果正在滚轮缩放、用户交互或 flyTo，不执行自动居中
    // 使用函数作用域的 isUserInteracting 和 isWheeling
    if (!viewer || isAdjusting || isUserInteracting || isFlyingTo || isWheeling) return;
    
    const now = performance.now();
    const cameraAdjustThrottle = window.CONFIG?.CAMERA_ADJUST?.throttle || 200; // 增加节流时间，减少频繁调用
    if (now - lastAdjustmentTime < cameraAdjustThrottle) {
      if (cameraAdjustmentFrameRef.current === null) {
        cameraAdjustmentFrameRef.current = requestAnimationFrame(lockCameraOnEarthCenter);
      }
      return;
    }
    lastAdjustmentTime = now;
    
    // 检查地球中心是否在屏幕中心（只在明显偏移时才调整，避免闪烁）
    const screenPosition = viewer.scene.cartesianToCanvasCoordinates(earthCenter);
    if (!screenPosition) {
      // 地球中心不在视野内，不调整
      cameraAdjustmentFrameRef.current = null;
      return;
    }
    
    const canvas = viewer.scene.canvas;
    const screenCenter = new Cesium.Cartesian2(canvas.width / 2, canvas.height / 2);
    const offsetX = screenPosition.x - screenCenter.x;
    const offsetY = screenPosition.y - screenCenter.y;
    const threshold = 50; // 增加到50像素阈值，只在明显偏移时才调整，避免干扰用户操作
    
    // 只在偏移超过阈值时才调整
    if (Math.abs(offsetX) > threshold || Math.abs(offsetY) > threshold) {
      isAdjusting = true;
      // 以当前 heading/pitch 为准，把相机锁定到地球中心的 range（确保地球中心在屏幕中心）
      const currentRange = Cesium.Cartesian3.magnitude(viewer.camera.position);
      const range = clampRange(currentRange);
      viewer.camera.lookAt(
        earthCenter,
        new Cesium.HeadingPitchRange(viewer.camera.heading, viewer.camera.pitch, range)
      );
      updateZoomLevel();
      setTimeout(() => { isAdjusting = false; }, 16);
    }
    
    cameraAdjustmentFrameRef.current = null;
  }

  /**
   * 初始化 Cesium
   */
  function initCesium() {
    // 如果已经初始化过，直接返回
    if (viewerRef.current) {
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
      const errorMsg = "Cesium库未加载，请检查资源路径或网络连接";
      if (window.toast) {
        window.toast.error(errorMsg, 5000);
      }
      throw new Error("Cesium library not loaded");
    }
    
    console.log("Initializing Cesium viewer...");
    
    const viewer = new Cesium.Viewer("cesiumContainer", {
      animation: false,
      timeline: false,
      geocoder: false,
      homeButton: false,
      sceneModePicker: false,
      baseLayerPicker: true,
      navigationHelpButton: false,
      fullscreenButton: false,
      imageryProvider: false,
    });

    viewer.imageryLayers.removeAll();

    // 性能优化配置 - 启用按需渲染以提高流畅度
    viewer.scene.requestRenderMode = true; // 启用按需渲染，只在需要时渲染
    viewer.scene.maximumRenderTimeChange = Infinity; // 允许无限时间变化
    viewer.scene.globe.tileCacheSize = 5000; // 增大瓦片缓存以提高性能
    viewer.scene.globe.preloadSiblings = true;
    viewer.scene.globe.preloadAncestors = true;
    
    // 使用CesiumHelpers配置瓦片加载（如果存在）
    if (window.CesiumHelpers) {
      window.CesiumHelpers.configureTileLoading(viewer);
      window.CesiumHelpers.configureRenderPerformance(viewer);
    }
    
    // 添加本地地图瓦片作为默认底图（优先使用本地瓦片）
    // 昌平区18级，其他地区15级
    const localTilesPath = "./tiles/{z}/{x}/{y}.png";
    const localImagery = new Cesium.UrlTemplateImageryProvider({
      url: localTilesPath,
      credit: "© OpenStreetMap contributors (北京市本地地图 - 昌平区18级/其他15级)",
      maximumLevel: 18, // 支持昌平区的18级瓦片
      minimumLevel: 0,
      enablePickFeatures: false,
      hasAlphaChannel: false
    });
    console.log("✅ 优先使用本地地图瓦片（北京市 - 昌平区18级/其他15级）");
    console.log("✅ 本地地图路径:", localTilesPath);
    
    // 优先添加本地地图层（第一个添加的图层会显示在最上层）
    const localLayer = viewer.imageryLayers.addImageryProvider(localImagery);
    
    // 确保本地图层在最上层（alpha=1.0表示完全不透明，优先显示）
    localLayer.alpha = 1.0;
    localLayer.show = true;
    
    // 静默处理404错误（本地瓦片不存在时）
    if (localLayer.imageryProvider.errorEvent) {
      localLayer.imageryProvider.errorEvent.addEventListener(function(error) {
        // 静默处理404错误，不显示在控制台
        // 本地瓦片不存在时，Cesium会自动显示空白或使用父级瓦片
      });
    }
    
    // 可选：添加在线瓦片作为后备（如果本地瓦片不存在）
    // 注意：这会增加网络请求，如果不需要可以注释掉
    // const onlineTilesPath = "https://a.tile.openstreetmap.org/{z}/{x}/{y}.png";
    // const onlineImagery = new Cesium.UrlTemplateImageryProvider({
    //   url: onlineTilesPath,
    //   credit: "© OpenStreetMap contributors (在线地图)",
    //   maximumLevel: 19,
    //   minimumLevel: 0,
    //   enablePickFeatures: false,
    //   hasAlphaChannel: false,
    // });
    // viewer.imageryLayers.addImageryProvider(onlineImagery);
    
    // 设置本地地图层的显示属性
    localLayer.alpha = 1.0;
    localLayer.brightness = 1.0;
    localLayer.contrast = 1.0;
    
    // 立即设置初始相机位置（先设置一个合理的初始位置，避免闪烁）
    // 先禁用自动居中，直到初始化完成
    isFlyingTo = true;
    cameraAdjustmentEnabled = false;
    
    // 设置初始相机位置：先定位到昌平公园附近（如果配置了默认位置）
    // 如果没有配置，使用地球中心
    if (defaultLocationId.value && locations.value.length > 0) {
      const defaultLoc = locations.value.find(l => l.id === defaultLocationId.value);
        if (defaultLoc) {
          // 使用默认位置的初始视角（正对着地面）
          const initialHeight = (defaultLoc.height || 1000) + EARTH_RADIUS;
          const targetPos = Cesium.Cartesian3.fromDegrees(defaultLoc.lon, defaultLoc.lat, 0);
          const heading = Cesium.Math.toRadians(defaultLoc.heading || 0);
          const pitch = Cesium.Math.toRadians(defaultLoc.pitch || -90); // 默认-90度，正对着地面
          
          // 使用lookAt确保相机正对着目标点（地面）
          viewer.camera.lookAt(
            targetPos,
            new Cesium.HeadingPitchRange(heading, pitch, initialHeight)
          );
      } else {
        // 如果没有找到默认位置，使用地球中心
        const initialDistance = MAX_DISTANCE * 0.8;
        viewer.camera.lookAt(
          earthCenter,
          new Cesium.HeadingPitchRange(0, 0, initialDistance)
        );
      }
    } else {
      // 如果没有配置默认位置，使用地球中心
      const initialDistance = MAX_DISTANCE * 0.8;
      viewer.camera.lookAt(
        earthCenter,
        new Cesium.HeadingPitchRange(0, 0, initialDistance)
      );
    }
    
    // 延迟初始化相机位置
    setTimeout(() => {
      function tryInitCamera() {
        if (!viewer || !viewer.scene || !viewer.scene.globe) {
          setTimeout(tryInitCamera, 100);
          return;
        }
        
        if (defaultLocationId.value) {
          const location = locations.value.find(l => l.id === defaultLocationId.value);
          if (location) {
            // location.height 是从地球表面的高度，需要加上地球半径
            const targetHeight = (location.height || 500) + EARTH_RADIUS;
            const safeHeight = Math.max(targetHeight, MIN_DISTANCE);
            
            // 标记正在 flyTo
            isFlyingTo = true;
            
            // 计算目标位置（地球表面的点）
            const targetPosition = Cesium.Cartesian3.fromDegrees(location.lon, location.lat, 0);
            
            // 使用lookAt直接设置相机位置和视角（正对着地面）
            const heading = Cesium.Math.toRadians(location.heading || 0);
            const pitch = Cesium.Math.toRadians(location.pitch || -90); // 默认-90度，正对着地面
            
            viewer.camera.flyTo({
              destination: Cesium.Cartesian3.fromDegrees(location.lon, location.lat, safeHeight),
              orientation: {
                heading: heading,
                pitch: pitch, // -90度，正对着地面
                roll: location.roll || 0.0,
              },
              duration: 2.0,
              complete: function() {
                // 确保目标点在屏幕中心，相机正对着地面
                const finalRange = Cesium.Cartesian3.magnitude(viewer.camera.position);
                
                // 使用lookAt确保目标点在屏幕中心，相机正对着地面
                viewer.camera.lookAt(
                  targetPosition,
                  new Cesium.HeadingPitchRange(heading, pitch, finalRange)
                );
                
                // 再次确保距离安全
                ensureSafeDistance();
                
                // 更新缩放级别显示
                if (window.updateZoomLevel) {
                  window.updateZoomLevel();
                }
                
                // 延迟恢复飞行状态（自动居中功能已禁用）
                setTimeout(() => {
                  isFlyingTo = false;
                  cameraAdjustmentEnabled = false; // 禁用自动居中，避免干扰用户操作
                  console.log("✅ 相机已定位到默认位置（昌平公园，高度1000米），相机正对着地面");
                }, 500);
              }
            });
          } else {
            // 如果没有默认位置，禁用自动居中
            setTimeout(() => {
              isFlyingTo = false;
              cameraAdjustmentEnabled = false; // 禁用自动居中，避免干扰用户操作
            }, 500);
          }
        } else {
          // 如果没有默认位置，禁用自动居中
          setTimeout(() => {
            isFlyingTo = false;
            cameraAdjustmentEnabled = false; // 禁用自动居中，避免干扰用户操作
          }, 500);
        }
      }
      setTimeout(tryInitCamera, 300);
    }, 100);
    
    // 确保 globe 已初始化
    if (viewer.scene && viewer.scene.globe) {
      viewer.scene.globe.show = true;
      
      // 延迟添加事件监听器
      setTimeout(() => {
        try {
          if (viewer.scene.globe && viewer.scene.globe.tileLoadErrorEvent) {
            viewer.scene.globe.tileLoadErrorEvent.addEventListener(function(error) {
              // 静默处理瓦片加载错误
            });
          }
        } catch (e) {
          console.warn("Failed to add tile load error listener:", e);
        }
      }, 100);
    }
    
    // 添加渲染错误监听器
    if (viewer.scene && viewer.scene.renderError) {
      try {
        viewer.scene.renderError.addEventListener(function(scene, error) {
          console.error("Scene render error:", error);
          // 只在严重错误时提示用户
          if (error && error.message && !error.message.includes('404')) {
            if (window.toast) {
              window.toast.warning("渲染错误: " + error.message, 3000);
            }
          }
        });
      } catch (e) {
        console.warn("Failed to add render error listener:", e);
      }
    }
    
    console.log("Cesium viewer initialized successfully");

    // 相机控制 - 使用 Cesium 标准配置
    setTimeout(() => {
      if (viewer.scene && viewer.scene.screenSpaceCameraController) {
        const controller = viewer.scene.screenSpaceCameraController;
        
        // 基本配置
        controller.enableRotate = true;
        controller.enableTranslate = true; // 启用平移，允许鼠标拖动平移
        controller.enableZoom = true;
        controller.enableTilt = true;
        controller.enableLook = true;
        
        // 设置缩放范围
        controller.minimumZoomDistance = MIN_DISTANCE;
        controller.maximumZoomDistance = MAX_DISTANCE;
        
        // 优化惯性参数（参考数字地球标准配置）
        controller.inertiaSpin = 0.9; // 旋转惯性，适中的平滑度
        controller.inertiaTranslate = 0.9;
        controller.inertiaZoom = 0.8; // 缩放惯性，适中的平滑度
        
        // 调整缩放参数（参考数字地球标准）
        // zoomFactor 控制每次滚轮滚动的缩放倍数
        if (controller.zoomFactor !== undefined) {
          controller.zoomFactor = 2.0; // 使用默认值，更符合标准
        }
        
        // 调整缩放速率
        if (controller.zoomRate !== undefined) {
          controller.zoomRate = 1.0; // 使用标准速率
        }
        
        controller.minimumCollisionTerrainHeight = 0; // 允许更接近地面
        
        // 设置事件类型（标准数字地球配置）
        controller.rotateEventTypes = [Cesium.CameraEventType.LEFT_DRAG];
        controller.translateEventTypes = [Cesium.CameraEventType.RIGHT_DRAG]; // 右键拖动平移
        controller.zoomEventTypes = [Cesium.CameraEventType.PINCH]; // 只保留触摸缩放，滚轮缩放使用自定义实现
        // 禁用倾斜功能，保持相机始终正对地面
        controller.tiltEventTypes = []; // 空数组，禁用所有倾斜操作
        controller.enableTilt = false; // 完全禁用倾斜
        
        // 监听相机变化，确保距离安全（节流处理，避免过度调用）
        // 注意：这个监听器会在下面的主监听器中统一处理，这里移除避免重复
        
        // 监听用户交互事件
        const canvas = viewer.canvas;
        // 注意：isUserInteracting 和 isWheeling 已在函数作用域顶部定义
        
        const handleInteractionStart = () => {
          isUserInteracting = true; // 使用函数作用域的变量
          // 取消任何待执行的自动调整
          if (cameraAdjustmentFrameRef.current !== null) {
            cancelAnimationFrame(cameraAdjustmentFrameRef.current);
            cameraAdjustmentFrameRef.current = null;
          }
          if (interactionTimeout) {
            clearTimeout(interactionTimeout);
            interactionTimeout = null;
          }
        };
        
        const handleInteractionEnd = () => {
          // 大幅延长用户交互检测时间，避免立即恢复自动调整
          // 滚轮操作后延长到30秒，确保用户选择的当前位置不会被重置
          if (interactionTimeout) {
            clearTimeout(interactionTimeout);
          }
          interactionTimeout = setTimeout(() => {
            isUserInteracting = false; // 使用函数作用域的变量
            interactionTimeout = null;
          }, 30000); // 增加到 30000ms（30秒），确保用户操作后不会立即触发自动居中
        };
        
        canvas.addEventListener('mousedown', handleInteractionStart);
        canvas.addEventListener('mouseup', handleInteractionEnd);
        canvas.addEventListener('mousemove', (e) => {
          // 只在按下鼠标时标记为交互（避免鼠标移动就触发）
          if (e.buttons !== 0) {
            handleInteractionStart();
          }
        }, { passive: true });
        
        // 监听右键拖动（平移）事件
        canvas.addEventListener('contextmenu', (e) => {
          e.preventDefault(); // 阻止右键菜单
        });
        
        // 滚轮缩放：以鼠标位置为目标，只改距离，地球不旋转
        let wheelTimeout = null;
        let zoomGestureTarget = null;   // 本轮缩放手势的固定目标点（Cartesian3）
        let zoomGestureHeading = null;  // 本轮手势的固定 heading（不随目标变）
        let zoomHeadingLockFrame = null;
        
        const lockHeadingDuringZoom = () => {
          if (zoomGestureHeading === null || !viewerRef.current?.camera) return;
          const camera = viewerRef.current.camera;
          camera.heading = zoomGestureHeading;
          camera.pitch = Cesium.Math.toRadians(-90);
          camera.roll = 0.0;
          zoomHeadingLockFrame = requestAnimationFrame(lockHeadingDuringZoom);
        };
        
        canvas.addEventListener('wheel', (e) => {
          e.preventDefault();
          e.stopPropagation();
          // 使用捕获阶段，确保在 Cesium 控制器之前处理，避免滚轮触发旋转
          handleInteractionStart();
          isWheeling = true;
          if (wheelTimeout) clearTimeout(wheelTimeout);
          
          const viewer = viewerRef.current;
          if (!viewer) {
            isWheeling = false;
            handleInteractionEnd();
            return;
          }
          if (Math.abs(e.deltaY) < 0.1) {
            isWheeling = false;
            handleInteractionEnd();
            return;
          }
          
          try {
            const savedPosition = viewer.camera.position.clone();
            const savedHeading = viewer.camera.heading;
            const savedPitch = viewer.camera.pitch;
            const savedRoll = viewer.camera.roll;
            
            // 手势开始时：固定目标点与 heading，并禁用控制器
            if (zoomGestureTarget === null) {
              zoomGestureTarget = new Cesium.Cartesian3();
              zoomGestureHeading = savedHeading;
              const canvasRect = canvas.getBoundingClientRect();
              const mouseX = e.clientX - canvasRect.left;
              const mouseY = e.clientY - canvasRect.top;
              const picked = viewer.camera.pickEllipsoid(new Cesium.Cartesian2(mouseX, mouseY), viewer.scene.globe.ellipsoid);
              if (picked) {
                Cesium.Cartesian3.clone(picked, zoomGestureTarget);
              } else {
                const center = new Cesium.Cartesian2(canvas.width / 2, canvas.height / 2);
                const centerPicked = viewer.camera.pickEllipsoid(center, viewer.scene.globe.ellipsoid);
                if (centerPicked) {
                  Cesium.Cartesian3.clone(centerPicked, zoomGestureTarget);
                } else {
                  // 鼠标和中心都未击中地表时，用相机正下方的地表点
                  const carto = Cesium.Cartographic.fromCartesian(viewer.camera.position);
                  carto.height = 0;
                  viewer.scene.globe.ellipsoid.cartographicToCartesian(carto, zoomGestureTarget);
                }
              }
              const ctrl = viewer.scene.screenSpaceCameraController;
              if (ctrl) {
                ctrl.enableRotate = ctrl.enableTranslate = ctrl.enableZoom = ctrl.enableTilt = ctrl.enableLook = false;
              }
              lockHeadingDuringZoom();
            }
            
            const zoomTarget = zoomGestureTarget;
            if (!zoomTarget || !isFinite(zoomTarget.x)) {
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            
            const targetNormal = viewer.scene.globe.ellipsoid.geodeticSurfaceNormal(zoomTarget, new Cesium.Cartesian3());
            if (!targetNormal || !isFinite(targetNormal.x)) {
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            
            const targetToCamera = Cesium.Cartesian3.subtract(savedPosition, zoomTarget, new Cesium.Cartesian3());
            const currentDistance = Cesium.Cartesian3.magnitude(targetToCamera);
            if (currentDistance < 0.001 || !isFinite(currentDistance)) {
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            
            const zoomFactor = e.deltaY > 0 ? 1.1 : 1 / 1.1;
            let newDistance = currentDistance * zoomFactor;
            newDistance = clampHeightAboveTarget(newDistance);
            if (!isFinite(newDistance) || newDistance < 0) {
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            if (Math.abs(newDistance - currentDistance) < 1) {
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            
            // 新位置：目标点 + 法向 * 距离（正对目标正上方）
            const newCameraPosition = Cesium.Cartesian3.add(
              zoomTarget,
              Cesium.Cartesian3.multiplyByScalar(targetNormal, newDistance, new Cesium.Cartesian3()),
              new Cesium.Cartesian3()
            );
            
            // 目标点的东-北-上，用于保持“北向”不转
            const enu = Cesium.Transforms.eastNorthUpToFixedFrame(zoomTarget, viewer.scene.globe.ellipsoid);
            const east = Cesium.Matrix4.getColumn(enu, 0, new Cesium.Cartesian3());
            const north = Cesium.Matrix4.getColumn(enu, 1, new Cesium.Cartesian3());
            // 视线向下 = -法向；up = 按 heading 旋转后的北（保持地图不转）
            const up = new Cesium.Cartesian3(
              Math.cos(zoomGestureHeading) * north.x + Math.sin(zoomGestureHeading) * east.x,
              Math.cos(zoomGestureHeading) * north.y + Math.sin(zoomGestureHeading) * east.y,
              Math.cos(zoomGestureHeading) * north.z + Math.sin(zoomGestureHeading) * east.z
            );
            Cesium.Cartesian3.normalize(up, up);
            const direction = Cesium.Cartesian3.negate(targetNormal, new Cesium.Cartesian3());
            Cesium.Cartesian3.normalize(direction, direction);
            
            try {
              viewer.camera.position = newCameraPosition;
              viewer.camera.direction = direction;
              viewer.camera.up = up;
              viewer.camera.right = Cesium.Cartesian3.cross(direction, up, new Cesium.Cartesian3());
              Cesium.Cartesian3.normalize(viewer.camera.right, viewer.camera.right);
            } catch (err) {
              viewer.camera.position = savedPosition;
              viewer.camera.heading = savedHeading;
              viewer.camera.pitch = savedPitch;
              viewer.camera.roll = savedRoll;
              isWheeling = false;
              handleInteractionEnd();
              return;
            }
            
            if (!isFinite(viewer.camera.position.x)) {
              viewer.camera.position = savedPosition;
              viewer.camera.heading = savedHeading;
              viewer.camera.pitch = savedPitch;
              viewer.camera.roll = savedRoll;
            }
            updateZoomLevel();
            if (viewer.scene.requestRenderMode) viewer.scene.requestRender();
            
            wheelTimeout = setTimeout(() => {
              isWheeling = false;
              if (zoomHeadingLockFrame != null) {
                cancelAnimationFrame(zoomHeadingLockFrame);
                zoomHeadingLockFrame = null;
              }
              zoomGestureTarget = null;
              zoomGestureHeading = null;
              const ctrl = viewer.scene.screenSpaceCameraController;
              if (ctrl) {
                ctrl.enableRotate = ctrl.enableTranslate = ctrl.enableZoom = ctrl.enableTilt = ctrl.enableLook = true;
              }
              if (viewer?.camera) {
                viewer.camera.heading = savedHeading;
                viewer.camera.pitch = savedPitch;
                viewer.camera.roll = savedRoll;
              }
              handleInteractionEnd();
            }, 200);
          } catch (error) {
            console.error('缩放错误:', error);
            isWheeling = false;
            handleInteractionEnd();
          }
        }, { passive: false, capture: true });
        
        // 暴露 isUserInteracting 和 isWheeling 到外部（使用函数作用域的变量）
        window.isUserInteracting = () => isUserInteracting;
        window.isWheeling = () => isWheeling;
        
        // 添加键盘快捷键支持（WASD移动相机）
        let keysPressed = {};
        const moveSpeed = 10000; // 移动速度（米/秒）
        const rotateSpeed = 0.02; // 旋转速度（弧度/帧）
        let lastMoveTime = performance.now();
        
        const handleKeyDown = (e) => {
          keysPressed[e.key.toLowerCase()] = true;
          
          // 防止默认行为（如页面滚动）
          if (['w', 'a', 's', 'd', 'q', 'e', 'r', 'f', 'arrowup', 'arrowdown', 'arrowleft', 'arrowright'].includes(e.key.toLowerCase())) {
            e.preventDefault();
          }
        };
        
        const handleKeyUp = (e) => {
          keysPressed[e.key.toLowerCase()] = false;
        };
        
        // 相机移动函数
        const moveCamera = () => {
          const viewer = viewerRef.current;
          // 使用函数作用域的 isWheeling 和 isUserInteracting
          if (!viewer || isWheeling || isUserInteracting || isFlyingTo) {
            requestAnimationFrame(moveCamera);
            return;
          }
          
          const now = performance.now();
          const deltaTime = Math.min((now - lastMoveTime) / 1000, 0.1); // 限制最大时间步长
          lastMoveTime = now;
          
          let moved = false;
          const camera = viewer.camera;
          const currentPosition = camera.position.clone();
          const currentHeading = camera.heading;
          const currentPitch = camera.pitch;
          
          // 计算相机的方向向量（使用相机的实际方向）
          // 获取相机的方向向量（从相机位置指向目标的方向）
          const direction = camera.direction.clone();
          Cesium.Cartesian3.normalize(direction, direction);
          
          // 计算右方向（方向向量与上向量的叉积）
          const right = Cesium.Cartesian3.cross(
            direction,
            camera.up,
            new Cesium.Cartesian3()
          );
          Cesium.Cartesian3.normalize(right, right);
          
          // 计算上方向（右方向与方向向量的叉积，确保垂直）
          const up = Cesium.Cartesian3.cross(
            right,
            direction,
            new Cesium.Cartesian3()
          );
          Cesium.Cartesian3.normalize(up, up);
          
          // W/S: 前进/后退（沿相机方向移动）
          if (keysPressed['w'] || keysPressed['arrowup']) {
            const move = Cesium.Cartesian3.multiplyByScalar(direction, moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          if (keysPressed['s'] || keysPressed['arrowdown']) {
            const move = Cesium.Cartesian3.multiplyByScalar(direction, -moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          
          // A/D: 左移/右移（沿右方向移动）
          if (keysPressed['a'] || keysPressed['arrowleft']) {
            const move = Cesium.Cartesian3.multiplyByScalar(right, -moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          if (keysPressed['d'] || keysPressed['arrowright']) {
            const move = Cesium.Cartesian3.multiplyByScalar(right, moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          
          // 上下移动（沿上方向移动）
          if (keysPressed['q']) {
            const move = Cesium.Cartesian3.multiplyByScalar(up, moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          if (keysPressed['e']) {
            const move = Cesium.Cartesian3.multiplyByScalar(up, -moveSpeed * deltaTime, new Cesium.Cartesian3());
            camera.position = Cesium.Cartesian3.add(currentPosition, move, new Cesium.Cartesian3());
            moved = true;
          }
          
          // R/F: 左旋转/右旋转（Q/E已用于上下移动）
          if (keysPressed['r']) {
            camera.heading = currentHeading + rotateSpeed;
            moved = true;
          }
          if (keysPressed['f']) {
            camera.heading = currentHeading - rotateSpeed;
            moved = true;
          }
          
          // 确保距离安全（节流处理）
          if (moved) {
            ensureSafeDistance();
            // 减少更新频率，提高流畅度（每100ms更新一次）
            const updateInterval = 100;
            if (now - lastMoveTime > updateInterval || (now - lastMoveTime) % updateInterval < 16) {
              updateZoomLevel();
            }
            handleInteractionStart();
            // 触发渲染
            const viewer = viewerRef.current;
            if (viewer && viewer.scene && viewer.scene.requestRenderMode) {
              viewer.scene.requestRender();
            }
          }
          
          requestAnimationFrame(moveCamera);
        };
        
        // 监听键盘事件
        window.addEventListener('keydown', handleKeyDown);
        window.addEventListener('keyup', handleKeyUp);
        
        // 启动相机移动循环
        moveCamera();
        
        console.log("Camera controller configured successfully (with WASD support)");
      }
    }, 200);

    // 将 updateZoomLevel 函数暴露到全局作用域
    window.updateZoomLevel = updateZoomLevel;
    
    // 相机调整将在 flyTo 完成后启用（在 tryInitCamera 中设置）
    // 这里不再单独启用，避免与初始化冲突
    
    // 优化相机变化监听 - 减少不必要的更新以提高流畅度
    let lastChangedTime = 0;
    let lastDistanceCheckTime = 0;
    let lastZoomUpdateTime = 0;
    let lastAutoAdjustTime = 0;
    
    viewer.camera.changed.addEventListener(function() {
      // 如果用户正在交互，跳过所有处理以提高流畅度
      if (window.isWheeling?.() || window.isUserInteracting?.()) {
        viewer.scene.requestRender(); // 触发渲染
        return;
      }
      
      const now = performance.now();
      
      // 节流：每帧最多处理一次（约16ms）
      if (now - lastChangedTime < 16) {
        return;
      }
      lastChangedTime = now;
      
      // 强制保持pitch = -90度（正对地面），防止倾斜操作改变视角
      // 只有在非用户交互时才强制设置，避免干扰缩放等操作
      if (!isUserInteracting && !isWheeling) {
        const currentPitch = viewer.camera.pitch;
        const targetPitch = Cesium.Math.toRadians(-90);
        // 如果pitch偏离-90度超过1度，强制恢复
        if (Math.abs(currentPitch - targetPitch) > Cesium.Math.toRadians(1)) {
          viewer.camera.pitch = targetPitch;
        }
      }
      
      // 确保距离安全（节流：每200ms检查一次）
      // 注意：用户交互时不会调用，避免干扰用户操作
      if (now - lastDistanceCheckTime > 200 && !isUserInteracting && !isWheeling) {
        ensureSafeDistance();
        lastDistanceCheckTime = now;
      }
      
      // 更新缩放级别显示（节流：每100ms更新一次，减少UI更新频率）
      if (now - lastZoomUpdateTime > 100) {
        updateZoomLevel();
        lastZoomUpdateTime = now;
      }
      
      // 自动居中功能已禁用，避免干扰用户操作
      // 如果需要在初始化后自动居中，可以通过配置启用
      // 目前默认禁用，用户操作后不会自动回到默认位置
      if (false && cameraAdjustmentEnabled && 
          cameraAdjustmentFrameRef.current === null && 
          now - lastAutoAdjustTime > 500) {
        cameraAdjustmentFrameRef.current = requestAnimationFrame(lockCameraOnEarthCenter);
        lastAutoAdjustTime = now;
      }
      
      // 触发渲染（requestRenderMode启用时需要）
      viewer.scene.requestRender();
    });
    
    viewer.camera.moveEnd.addEventListener(function() {
      // 移动结束后检查距离和更新显示（只在非用户交互时）
      if (!isUserInteracting && !isWheeling) {
        ensureSafeDistance();
      }
      updateZoomLevel();
      viewer.scene.requestRender();
    });
    
    // 初始化缩放级别显示
    updateZoomLevel();
    
    // 使用单一的requestAnimationFrame更新缩放级别（优化性能）
    let lastRAFUpdate = 0;
    function rafUpdateZoom() {
      const now = performance.now();
      // 只在用户交互时频繁更新，否则降低频率
      const updateInterval = window.isUserInteracting?.() ? 50 : 200;
      if (now - lastRAFUpdate > updateInterval) {
        if (viewerRef.current && viewerRef.current.camera && !window.isWheeling?.()) {
          updateZoomLevel();
        }
        lastRAFUpdate = now;
      }
      requestAnimationFrame(rafUpdateZoom);
    }
    requestAnimationFrame(rafUpdateZoom);
    
    // 定期检查距离（降低频率，避免过度干扰）
    // 注意：用户交互时完全跳过，避免干扰用户操作
    setInterval(() => {
      if (!isUserInteracting && !isWheeling && !isFlyingTo) {
        ensureSafeDistance();
      }
    }, 2000);
    
    viewerRef.current = viewer;
    return viewer;
  }

  /**
   * 设置 flyTo 状态（供外部调用）
   */
  function setFlyingTo(flying) {
    isFlyingTo = flying;
  }

  return {
    initCesium,
    updateZoomLevel,
    UAV_COLORS,
    setFlyingTo
  };
}
