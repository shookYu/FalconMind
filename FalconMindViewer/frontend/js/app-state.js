/**
 * 应用状态管理
 * 定义所有响应式数据和计算属性
 */
function createAppState() {
  const { ref, reactive, computed } = Vue;
  
  // 响应式数据
  const uavStates = reactive({});
  const selectedUavId = ref(null);
  const missions = reactive({});
  const wsStatus = ref("connecting");
  const zoomLevel = ref("级别 0 | 1.00x | 0 m");
  const locations = ref([]);
  const selectedLocationId = ref(null);
  const defaultLocationId = ref(null);
  
  // 计算属性
  const selectedUavInfo = computed(() => {
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

  const uavList = computed(() => {
    return Object.keys(uavStates);
  });

  const missionList = computed(() => {
    return Object.values(missions);
  });

  // 回放状态
  const playbackState = reactive({
    isPlaying: false,
    currentTime: 0,
    playbackSpeed: 1.0,
    startTime: null,
    endTime: null
  });

  return {
    uavStates,
    selectedUavId,
    missions,
    wsStatus,
    zoomLevel,
    locations,
    selectedLocationId,
    defaultLocationId,
    playbackState,
    selectedUavInfo,
    uavList,
    missionList
  };
}
