/**
 * 轨迹回放管理模块
 */
function createPlaybackManager(state, viewerRef, uavEntitiesRef, trajectoryHistoryRef) {
  const { playbackState } = state;

  /**
   * 开始回放
   */
  function startPlayback(uavId) {
    const trajectoryHistory = trajectoryHistoryRef.current;
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

  /**
   * 停止回放
   */
  function stopPlayback() {
    playbackState.isPlaying = false;
  }

  /**
   * 回放动画
   */
  function playbackAnimation(uavId) {
    const trajectoryHistory = trajectoryHistoryRef.current;
    const uavEntities = uavEntitiesRef.current;
    
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

  return {
    startPlayback,
    stopPlayback,
    playbackAnimation
  };
}
