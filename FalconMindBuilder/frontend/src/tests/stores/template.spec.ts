import { describe, it, expect, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useTemplateStore } from '../src/stores/template'

describe('Template Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should have built-in templates', () => {
    const store = useTemplateStore()
    
    expect(store.templates.length).toBeGreaterThan(0)
    
    // 检查是否有内置模板
    const basicSearch = store.getTemplateById('basic_search')
    expect(basicSearch).toBeDefined()
    expect(basicSearch?.name).toBe('基础搜索')
  })

  it('should filter templates by category', () => {
    const store = useTemplateStore()
    
    const searchTemplates = store.filteredTemplates({
      category: 'search'
    })
    
    expect(searchTemplates.length).toBeGreaterThan(0)
    searchTemplates.forEach(template => {
      expect(template.category).toBe('search')
    })
  })

  it('should filter templates by complexity', () => {
    const store = useTemplateStore()
    
    const simpleTemplates = store.filteredTemplates({
      complexity: 'simple'
    })
    
    simpleTemplates.forEach(template => {
      expect(template.complexity).toBe('simple')
    })
  })

  it('should search templates', () => {
    const store = useTemplateStore()
    
    const results = store.filteredTemplates({
      search: '搜索'
    })
    
    expect(results.length).toBeGreaterThan(0)
  })

  it('should instantiate template with parameters', () => {
    const store = useTemplateStore()
    
    const template = store.getTemplateById('basic_search')
    expect(template).toBeDefined()
    
    if (template) {
      const flowData = store.instantiateTemplate(
        template,
        {
          altitude: 150,
          speed: 10,
          pattern: 'spiral',
          enableDetection: true
        },
        '我的搜索任务'
      )
      
      expect(flowData.name).toBe('我的搜索任务')
      expect(flowData.nodes.length).toBeGreaterThan(0)
      expect(flowData.templateId).toBe('basic_search')
    }
  })

  it('should add custom template', () => {
    const store = useTemplateStore()
    
    const newTemplate = store.addTemplate({
      name: '自定义模板',
      category: 'custom',
      description: '测试模板',
      complexity: 'simple',
      version: '1.0.0',
      tags: ['测试'],
      parameters: [],
      defaultParams: {},
      nodes: [],
      edges: []
    })
    
    expect(newTemplate.id).toMatch(/^custom_/)
    expect(store.templates.length).toBeGreaterThan(4)
  })
})
