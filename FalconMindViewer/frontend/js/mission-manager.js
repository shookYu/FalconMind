/**
 * 任务管理模块
 * 处理任务的 CRUD 操作
 */
function createMissionManager(state, visualizationManager) {
  const { missions, uavList } = state;

  /**
   * 更新任务列表
   */
  async function updateMissionList() {
    try {
      const api = window.api;
      if (!api) {
        console.error('API service not available');
        return;
      }
      const data = await api.getMissions();
      
      // 清空并重新填充
      Object.keys(missions).forEach(key => delete missions[key]);
      if (data.missions) {
        data.missions.forEach(mission => {
          missions[mission.mission_id] = mission;
          // 如果任务包含搜索区域信息，显示搜索区域
          if (mission.payload && mission.payload.search_area) {
            visualizationManager.updateSearchAreaForMission(mission.mission_id, mission.payload.search_area);
          }
          // 如果任务包含航点信息，显示搜索路径
          if (mission.payload && mission.payload.waypoints) {
            visualizationManager.updateSearchPath(mission.mission_id, mission.payload.waypoints);
          }
        });
      }
    } catch (e) {
      console.error("Failed to fetch missions", e);
    }
  }

  /**
   * 创建测试任务
   */
  async function createTestMission() {
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  /**
   * 下发任务
   */
  async function dispatchMission(missionId) {
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  /**
   * 暂停任务
   */
  async function pauseMission(missionId) {
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  /**
   * 恢复任务
   */
  async function resumeMission(missionId) {
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  /**
   * 取消任务
   */
  async function cancelMission(missionId) {
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  /**
   * 删除任务
   */
  async function deleteMission(missionId) {
    if (!confirm("Are you sure you want to delete this mission?")) {
      return;
    }
    try {
      const api = window.api;
      if (!api) {
        throw new Error('API service not available');
      }
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

  return {
    updateMissionList,
    createTestMission,
    dispatchMission,
    pauseMission,
    resumeMission,
    cancelMission,
    deleteMission
  };
}
