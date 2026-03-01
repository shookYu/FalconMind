/**
 * 工具栏操作函数
 * 提供导航、回放、视图等工具栏功能
 */
function createToolbarActions(state, viewerRef, playbackManager, locationManager) {
  const { selectedUavId, uavStates, playbackState } = state;

  /**
   * 聚焦选中的UAV
   */
  function focusSelectedUav() {
    const viewer = viewerRef.current;
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
    }
  }

  /**
   * 重置相机到默认位置
   */
  function resetCamera() {
    const viewer = viewerRef.current;
    if (viewer && state.defaultLocationId.value) {
      const location = state.locations.value.find(l => l.id === state.defaultLocationId.value);
      if (location) {
        locationManager.flyToLocation(location.id);
        if (window.toast) {
          window.toast.info('相机已重置到默认位置');
        }
      }
    }
  }

  /**
   * 居中显示所有UAV
   */
  function centerAllUavs() {
    const viewer = viewerRef.current;
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

  /**
   * 取消选择
   */
  function clearSelection() {
    selectedUavId.value = null;
    if (window.toast) {
      window.toast.info('已取消选择');
    }
  }

  /**
   * 切换回放
   */
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

  /**
   * 加快回放速度
   */
  function speedUpPlayback() {
    playbackState.playbackSpeed = Math.min(playbackState.playbackSpeed * 1.5, 10);
    if (window.toast) {
      window.toast.info(`回放速度: ${playbackState.playbackSpeed.toFixed(1)}x`);
    }
  }

  /**
   * 减慢回放速度
   */
  function speedDownPlayback() {
    playbackState.playbackSpeed = Math.max(playbackState.playbackSpeed / 1.5, 0.1);
    if (window.toast) {
      window.toast.info(`回放速度: ${playbackState.playbackSpeed.toFixed(1)}x`);
    }
  }

  return {
    focusSelectedUav,
    resetCamera,
    centerAllUavs,
    clearSelection,
    togglePlayback,
    speedUpPlayback,
    speedDownPlayback
  };
}
