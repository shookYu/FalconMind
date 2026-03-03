import { describe, it, expect, beforeEach } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useFlowStore } from '../src/stores/flow'
import type { Node, Edge } from '@vue-flow/core'

describe('Flow Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should add a node', () => {
    const store = useFlowStore()
    
    const node: Node = {
      id: 'node-1',
      type: 'action',
      position: { x: 100, y: 100 },
      data: { type: 'search_area', label: '搜索区域' }
    }
    
    store.addNode(node)
    
    expect(store.nodes).toHaveLength(1)
    expect(store.nodes[0].id).toBe('node-1')
  })

  it('should remove a node and its edges', () => {
    const store = useFlowStore()
    
    // 添加节点
    store.addNode({
      id: 'node-1',
      type: 'trigger',
      position: { x: 100, y: 100 },
      data: { type: 'mission_start', label: '开始' }
    } as Node)
    
    store.addNode({
      id: 'node-2',
      type: 'action',
      position: { x: 300, y: 100 },
      data: { type: 'search_area', label: '搜索' }
    } as Node)
    
    // 添加边
    store.addEdge({
      id: 'edge-1',
      source: 'node-1',
      target: 'node-2'
    } as Edge)
    
    // 删除节点
    store.removeNode('node-1')
    
    expect(store.nodes).toHaveLength(1)
    expect(store.edges).toHaveLength(0)
  })

  it('should update node data', () => {
    const store = useFlowStore()
    
    store.addNode({
      id: 'node-1',
      type: 'action',
      position: { x: 100, y: 100 },
      data: { type: 'search_area', label: '搜索区域' }
    } as Node)
    
    store.updateNode('node-1', {
      data: { type: 'search_area', label: '更新后的名称' }
    })
    
    expect(store.nodes[0].data.label).toBe('更新后的名称')
  })

  it('should support undo/redo', () => {
    const store = useFlowStore()
    
    expect(store.canUndo).toBe(false)
    expect(store.canRedo).toBe(false)
    
    // 添加节点
    store.addNode({
      id: 'node-1',
      type: 'trigger',
      position: { x: 100, y: 100 },
      data: { type: 'mission_start', label: '开始' }
    } as Node)
    
    expect(store.canUndo).toBe(true)
    expect(store.nodes).toHaveLength(1)
    
    // 撤销
    store.undo()
    expect(store.nodes).toHaveLength(0)
    expect(store.canRedo).toBe(true)
    
    // 重做
    store.redo()
    expect(store.nodes).toHaveLength(1)
  })

  it('should clear flow', () => {
    const store = useFlowStore()
    
    store.addNode({
      id: 'node-1',
      type: 'trigger',
      position: { x: 100, y: 100 },
      data: {}
    } as Node)
    
    store.clearFlow()
    
    expect(store.nodes).toHaveLength(0)
    expect(store.edges).toHaveLength(0)
    expect(store.selectedNode).toBeNull()
  })
})
