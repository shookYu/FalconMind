import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { FlowTemplate, TemplateCategory, TemplateComplexity, TemplateFilter } from '@types/template'

// 内置模板数据
const builtInTemplates: FlowTemplate[] = [
  {
    id: 'basic_search',
    name: '基础搜索',
    category: 'search',
    description: '标准的区域搜索任务，适用于大多数搜索场景',
    icon: 'search',
    complexity: 'simple',
    version: '1.0.0',
    author: 'FalconMind',
    tags: ['搜索', '基础', '推荐'],
    parameters: [
      {
        name: 'area',
        type: 'area',
        label: '搜索区域',
        description: '在地图上绘制搜索区域',
        required: true
      },
      {
        name: 'altitude',
        type: 'number',
        label: '飞行高度',
        description: '飞行高度（米）',
        required: true,
        default: 100,
        min: 10,
        max: 500,
        step: 10
      },
      {
        name: 'speed',
        type: 'number',
        label: '飞行速度',
        description: '飞行速度（m/s）',
        required: true,
        default: 8,
        min: 1,
        max: 20,
        step: 0.5
      },
      {
        name: 'pattern',
        type: 'select',
        label: '搜索模式',
        description: '选择搜索航线模式',
        required: true,
        default: 'lawn_mower',
        options: [
          { label: '网格搜索', value: 'lawn_mower' },
          { label: '螺旋搜索', value: 'spiral' },
          { label: '扇形搜索', value: 'sector' }
        ]
      },
      {
        name: 'enableDetection',
        type: 'boolean',
        label: '启用目标检测',
        description: '是否启用 AI 目标检测',
        required: true,
        default: true
      },
      {
        name: 'detectionClasses',
        type: 'multiselect',
        label: '检测类别',
        description: '选择要检测的目标类别',
        required: false,
        default: ['person', 'vehicle'],
        options: [
          { label: '人员', value: 'person' },
          { label: '车辆', value: 'vehicle' },
          { label: '火灾', value: 'fire' },
          { label: '烟雾', value: 'smoke' }
        ]
      }
    ],
    defaultParams: {
      altitude: 100,
      speed: 8,
      pattern: 'lawn_mower',
      enableDetection: true,
      detectionClasses: ['person', 'vehicle'],
      detectionThreshold: 0.5
    },
    nodes: [
      {
        id: 'trigger_1',
        type: 'trigger',
        subtype: 'mission_start',
        label: '任务开始',
        position: { x: 100, y: 100 },
        data: { type: 'mission_start' }
      },
      {
        id: 'action_1',
        type: 'action',
        subtype: 'search_area',
        label: '搜索区域',
        position: { x: 300, y: 100 },
        data: { type: 'search_area' }
      },
      {
        id: 'action_2',
        type: 'action',
        subtype: 'return_home',
        label: '返航',
        position: { x: 500, y: 100 },
        data: { type: 'return_home' }
      }
    ],
    edges: [
      { id: 'edge_1', source: 'trigger_1', target: 'action_1' },
      { id: 'edge_2', source: 'action_1', target: 'action_2' }
    ],
    createdAt: '2024-03-01T00:00:00Z',
    updatedAt: '2024-03-01T00:00:00Z'
  },
  {
    id: 'forest_fire_search',
    name: '森林火灾搜索',
    category: 'emergency',
    description: '专门针对森林火灾场景的搜索任务，包含火灾和烟雾检测',
    icon: 'fire',
    complexity: 'medium',
    version: '1.0.0',
    author: 'FalconMind',
    tags: ['应急', '火灾', '搜索'],
    parameters: [
      {
        name: 'area',
        type: 'area',
        label: '搜索区域',
        description: '在地图上绘制搜索区域',
        required: true
      },
      {
        name: 'altitude',
        type: 'number',
        label: '飞行高度',
        description: '建议高度 120-150 米以获得更好视野',
        required: true,
        default: 120,
        min: 50,
        max: 500,
        step: 10
      },
      {
        name: 'speed',
        type: 'number',
        label: '飞行速度',
        required: true,
        default: 6,
        min: 3,
        max: 15,
        step: 0.5
      }
    ],
    defaultParams: {
      altitude: 120,
      speed: 6,
      pattern: 'spiral',
      enableDetection: true,
      detectionClasses: ['fire', 'smoke'],
      detectionThreshold: 0.7
    },
    nodes: [
      {
        id: 'trigger_1',
        type: 'trigger',
        subtype: 'mission_start',
        label: '任务开始',
        position: { x: 100, y: 100 },
        data: { type: 'mission_start' }
      },
      {
        id: 'action_1',
        type: 'action',
        subtype: 'search_area',
        label: '搜索火灾',
        position: { x: 300, y: 100 },
        data: { type: 'search_area' }
      },
      {
        id: 'condition_1',
        type: 'condition',
        subtype: 'target_detected',
        label: '发现火情?',
        position: { x: 500, y: 100 },
        data: { type: 'target_detected' }
      },
      {
        id: 'action_2',
        type: 'action',
        subtype: 'take_photo',
        label: '拍照记录',
        position: { x: 700, y: 50 },
        data: { type: 'take_photo' }
      },
      {
        id: 'action_3',
        type: 'action',
        subtype: 'send_message',
        label: '发送告警',
        position: { x: 700, y: 150 },
        data: { type: 'send_message' }
      },
      {
        id: 'action_4',
        type: 'action',
        subtype: 'return_home',
        label: '返航',
        position: { x: 900, y: 100 },
        data: { type: 'return_home' }
      }
    ],
    edges: [
      { id: 'edge_1', source: 'trigger_1', target: 'action_1' },
      { id: 'edge_2', source: 'action_1', target: 'condition_1' },
      { id: 'edge_3', source: 'condition_1', target: 'action_2' },
      { id: 'edge_4', source: 'condition_1', target: 'action_3' },
      { id: 'edge_5', source: 'action_2', target: 'action_4' },
      { id: 'edge_6', source: 'action_3', target: 'action_4' }
    ],
    createdAt: '2024-03-01T00:00:00Z',
    updatedAt: '2024-03-01T00:00:00Z'
  },
  {
    id: 'perimeter_patrol',
    name: '周界巡逻',
    category: 'patrol',
    description: '沿区域边界进行巡逻监控',
    icon: 'patrol',
    complexity: 'simple',
    version: '1.0.0',
    author: 'FalconMind',
    tags: ['巡逻', '监控', '周界'],
    parameters: [
      {
        name: 'area',
        type: 'area',
        label: '巡逻区域',
        description: '绘制巡逻区域边界',
        required: true
      },
      {
        name: 'altitude',
        type: 'number',
        label: '巡逻高度',
        required: true,
        default: 80,
        min: 20,
        max: 300,
        step: 10
      },
      {
        name: 'speed',
        type: 'number',
        label: '巡逻速度',
        required: true,
        default: 5,
        min: 2,
        max: 15,
        step: 0.5
      },
      {
        name: 'loops',
        type: 'number',
        label: '巡逻圈数',
        description: '巡逻多少圈后自动返航',
        required: true,
        default: 3,
        min: 1,
        max: 10,
        step: 1
      }
    ],
    defaultParams: {
      altitude: 80,
      speed: 5,
      loops: 3,
      pattern: 'perimeter',
      enableDetection: true,
      detectionClasses: ['person', 'vehicle'],
      detectionThreshold: 0.6
    },
    nodes: [
      {
        id: 'trigger_1',
        type: 'trigger',
        subtype: 'mission_start',
        label: '任务开始',
        position: { x: 100, y: 100 },
        data: { type: 'mission_start' }
      },
      {
        id: 'action_1',
        type: 'action',
        subtype: 'patrol_area',
        label: '周界巡逻',
        position: { x: 300, y: 100 },
        data: { type: 'patrol_area' }
      },
      {
        id: 'action_2',
        type: 'action',
        subtype: 'return_home',
        label: '返航',
        position: { x: 500, y: 100 },
        data: { type: 'return_home' }
      }
    ],
    edges: [
      { id: 'edge_1', source: 'trigger_1', target: 'action_1' },
      { id: 'edge_2', source: 'action_1', target: 'action_2' }
    ],
    createdAt: '2024-03-01T00:00:00Z',
    updatedAt: '2024-03-01T00:00:00Z'
  },
  {
    id: 'powerline_inspection',
    name: '电力巡检',
    category: 'inspection',
    description: '沿电力线路进行巡检，检测异常',
    icon: 'power',
    complexity: 'advanced',
    version: '1.0.0',
    author: 'FalconMind',
    tags: ['巡检', '电力', '基础设施'],
    parameters: [
      {
        name: 'waypoints',
        type: 'area',
        label: '巡检路线',
        description: '沿电力线绘制巡检路线',
        required: true
      },
      {
        name: 'altitude',
        type: 'number',
        label: '巡检高度',
        description: '相对地面高度',
        required: true,
        default: 50,
        min: 20,
        max: 150,
        step: 5
      },
      {
        name: 'speed',
        type: 'number',
        label: '巡检速度',
        required: true,
        default: 4,
        min: 2,
        max: 10,
        step: 0.5
      },
      {
        name: 'hoverTime',
        type: 'number',
        label: '塔位悬停时间',
        description: '在每个电塔位置悬停多久（秒）',
        required: true,
        default: 5,
        min: 0,
        max: 30,
        step: 1
      }
    ],
    defaultParams: {
      altitude: 50,
      speed: 4,
      hoverTime: 5,
      pattern: 'waypoint',
      enableDetection: true,
      detectionClasses: ['building'],
      detectionThreshold: 0.5
    },
    nodes: [
      {
        id: 'trigger_1',
        type: 'trigger',
        subtype: 'mission_start',
        label: '任务开始',
        position: { x: 100, y: 100 },
        data: { type: 'mission_start' }
      },
      {
        id: 'action_1',
        type: 'action',
        subtype: 'inspect_line',
        label: '电力巡检',
        position: { x: 300, y: 100 },
        data: { type: 'inspect_line' }
      },
      {
        id: 'action_2',
        type: 'action',
        subtype: 'return_home',
        label: '返航',
        position: { x: 500, y: 100 },
        data: { type: 'return_home' }
      }
    ],
    edges: [
      { id: 'edge_1', source: 'trigger_1', target: 'action_1' },
      { id: 'edge_2', source: 'action_1', target: 'action_2' }
    ],
    createdAt: '2024-03-01T00:00:00Z',
    updatedAt: '2024-03-01T00:00:00Z'
  },
  {
    id: 'rescue_search',
    name: '搜救任务',
    category: 'emergency',
    description: '人员搜救任务，包含发现目标后的悬停和告警',
    icon: 'rescue',
    complexity: 'advanced',
    version: '1.0.0',
    author: 'FalconMind',
    tags: ['应急', '搜救', '人员'],
    parameters: [
      {
        name: 'area',
        type: 'area',
        label: '搜救区域',
        description: '绘制搜救区域',
        required: true
      },
      {
        name: 'altitude',
        type: 'number',
        label: '搜索高度',
        description: '建议 80-120 米',
        required: true,
        default: 100,
        min: 30,
        max: 200,
        step: 10
      },
      {
        name: 'speed',
        type: 'number',
        label: '搜索速度',
        required: true,
        default: 6,
        min: 3,
        max: 12,
        step: 0.5
      },
      {
        name: 'hoverOnFind',
        type: 'boolean',
        label: '发现目标后悬停',
        required: true,
        default: true
      }
    ],
    defaultParams: {
      altitude: 100,
      speed: 6,
      hoverOnFind: true,
      pattern: 'lawn_mower',
      enableDetection: true,
      detectionClasses: ['person'],
      detectionThreshold: 0.6
    },
    nodes: [
      {
        id: 'trigger_1',
        type: 'trigger',
        subtype: 'mission_start',
        label: '任务开始',
        position: { x: 100, y: 100 },
        data: { type: 'mission_start' }
      },
      {
        id: 'action_1',
        type: 'action',
        subtype: 'search_area',
        label: '搜索区域',
        position: { x: 300, y: 100 },
        data: { type: 'search_area' }
      },
      {
        id: 'condition_1',
        type: 'condition',
        subtype: 'target_detected',
        label: '发现人员?',
        position: { x: 500, y: 100 },
        data: { type: 'target_detected' }
      },
      {
        id: 'action_2',
        type: 'action',
        subtype: 'hover',
        label: '悬停观察',
        position: { x: 700, y: 50 },
        data: { type: 'hover', duration: 30 }
      },
      {
        id: 'action_3',
        type: 'action',
        subtype: 'take_photo',
        label: '拍照取证',
        position: { x: 700, y: 150 },
        data: { type: 'take_photo' }
      },
      {
        id: 'action_4',
        type: 'action',
        subtype: 'return_home',
        label: '返航',
        position: { x: 900, y: 100 },
        data: { type: 'return_home' }
      }
    ],
    edges: [
      { id: 'edge_1', source: 'trigger_1', target: 'action_1' },
      { id: 'edge_2', source: 'action_1', target: 'condition_1' },
      { id: 'edge_3', source: 'condition_1', target: 'action_2' },
      { id: 'edge_4', source: 'condition_1', target: 'action_3' },
      { id: 'edge_5', source: 'action_2', target: 'action_4' },
      { id: 'edge_6', source: 'action_3', target: 'action_4' }
    ],
    createdAt: '2024-03-01T00:00:00Z',
    updatedAt: '2024-03-01T00:00:00Z'
  }
]

export const useTemplateStore = defineStore('template', () => {
  // State
  const templates = ref<FlowTemplate[]>([...builtInTemplates])
  const selectedTemplate = ref<FlowTemplate | null>(null)
  const loading = ref(false)

  // Getters
  const categories = computed(() => {
    const cats = new Set(templates.value.map(t => t.category))
    return Array.from(cats)
  })

  const tags = computed(() => {
    const tagSet = new Set<string>()
    templates.value.forEach(t => {
      t.tags.forEach(tag => tagSet.add(tag))
    })
    return Array.from(tagSet)
  })

  const filteredTemplates = computed(() => {
    return (filter: TemplateFilter) => {
      return templates.value.filter(template => {
        // 分类筛选
        if (filter.category && template.category !== filter.category) {
          return false
        }

        // 难度筛选
        if (filter.complexity && template.complexity !== filter.complexity) {
          return false
        }

        // 搜索筛选
        if (filter.search) {
          const searchLower = filter.search.toLowerCase()
          const matchName = template.name.toLowerCase().includes(searchLower)
          const matchDesc = template.description.toLowerCase().includes(searchLower)
          const matchTags = template.tags.some(t => t.toLowerCase().includes(searchLower))
          if (!matchName && !matchDesc && !matchTags) {
            return false
          }
        }

        // 标签筛选
        if (filter.tags && filter.tags.length > 0) {
          if (!filter.tags.some(tag => template.tags.includes(tag))) {
            return false
          }
        }

        return true
      })
    }
  })

  // Actions
  const setSelectedTemplate = (template: FlowTemplate | null) => {
    selectedTemplate.value = template
  }

  const getTemplateById = (id: string): FlowTemplate | undefined => {
    return templates.value.find(t => t.id === id)
  }

  // 实例化模板为 Flow
  const instantiateTemplate = (
    template: FlowTemplate,
    params: Record<string, any>,
    name: string
  ) => {
    // 合并默认参数和用户参数
    const mergedParams = { ...template.defaultParams, ...params }

    // 创建节点副本并应用参数
    const nodes = template.nodes.map(node => {
      const newNode = {
        ...node,
        id: `${node.id}_${Date.now()}`,
        data: { ...node.data }
      }

      // 如果是搜索节点，应用搜索参数
      if (node.subtype === 'search_area') {
        newNode.data.config = {
          area: mergedParams.area || [],
          altitude: mergedParams.altitude,
          speed: mergedParams.speed,
          pattern: mergedParams.pattern,
          detection: mergedParams.enableDetection ? {
            enabled: true,
            model: mergedParams.detectionModel || 'yolov8n',
            classes: mergedParams.detectionClasses || ['person'],
            threshold: mergedParams.detectionThreshold || 0.5
          } : { enabled: false }
        }
      }

      return newNode
    })

    // 创建边线副本，更新 source 和 target
    const nodeIdMap = new Map(template.nodes.map((n, i) => [n.id, nodes[i].id]))
    const edges = template.edges.map(edge => ({
      ...edge,
      id: `edge_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      source: nodeIdMap.get(edge.source) || edge.source,
      target: nodeIdMap.get(edge.target) || edge.target
    }))

    return {
      name,
      description: `从模板 "${template.name}" 创建`,
      nodes,
      edges,
      templateId: template.id
    }
  }

  // 添加自定义模板
  const addTemplate = (template: Omit<FlowTemplate, 'id' | 'createdAt' | 'updatedAt'>) => {
    const newTemplate: FlowTemplate = {
      ...template,
      id: `custom_${Date.now()}`,
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString()
    }
    templates.value.push(newTemplate)
    return newTemplate
  }

  // 删除自定义模板
  const removeTemplate = (id: string) => {
    const index = templates.value.findIndex(t => t.id === id)
    if (index > -1 && templates.value[index].category === 'custom') {
      templates.value.splice(index, 1)
      return true
    }
    return false
  }

  return {
    templates,
    selectedTemplate,
    loading,
    categories,
    tags,
    filteredTemplates,
    setSelectedTemplate,
    getTemplateById,
    instantiateTemplate,
    addTemplate,
    removeTemplate
  }
})