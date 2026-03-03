/**
 * Flow 模板类型定义
 */

// 模板类别
export type TemplateCategory = 'search' | 'patrol' | 'inspection' | 'emergency' | 'custom'

// 模板难度
export type TemplateComplexity = 'simple' | 'medium' | 'advanced'

// 模板配置参数
export interface TemplateParameter {
  name: string
  type: 'string' | 'number' | 'boolean' | 'select' | 'multiselect' | 'area'
  label: string
  description?: string
  required: boolean
  default?: any
  options?: { label: string; value: any }[]
  min?: number
  max?: number
  step?: number
}

// 模板节点
export interface TemplateNode {
  id: string
  type: 'trigger' | 'action' | 'condition'
  subtype: string
  label: string
  position: { x: number; y: number }
  data: Record<string, any>
}

// 模板边线
export interface TemplateEdge {
  id: string
  source: string
  target: string
}

// 模板定义
export interface FlowTemplate {
  id: string
  name: string
  category: TemplateCategory
  description: string
  icon?: string
  complexity: TemplateComplexity
  version: string
  author?: string
  tags: string[]
  
  // 参数配置
  parameters: TemplateParameter[]
  
  // 默认参数值
  defaultParams: Record<string, any>
  
  // 节点和边线模板
  nodes: TemplateNode[]
  edges: TemplateEdge[]
  
  // 预览配置
  preview?: {
    image?: string
    description?: string
  }
  
  // 创建时间
  createdAt: string
  updatedAt: string
}

// 模板筛选选项
export interface TemplateFilter {
  category?: TemplateCategory
  complexity?: TemplateComplexity
  search?: string
  tags?: string[]
}

// 实例化参数
export interface TemplateInstanceParams {
  templateId: string
  name: string
  description?: string
  parameters: Record<string, any>
}
