/**
 * 视图管理模块
 * 处理视图保存和恢复
 */
function createViewManager(viewerRef) {
  /**
   * 保存当前视图
   */
  function saveView() {
    const viewer = viewerRef.current;
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

  /**
   * 恢复保存的视图
   */
  function restoreView() {
    const viewer = viewerRef.current;
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
        console.error('Failed to restore view:', e);
        if (window.toast) {
          window.toast.error('恢复视图失败: ' + e.message);
        }
      }
    }
  }

  return {
    saveView,
    restoreView
  };
}
