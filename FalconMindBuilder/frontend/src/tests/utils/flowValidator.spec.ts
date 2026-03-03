import { describe, it, expect } from 'vitest'
import { validateFlow } from '../src/utils/flowValidator'

describe('Flow Validator', () => {
  it('should validate empty flow', () => {
    const result = validateFlow([], [])
    
    expect(result.valid).toBe(false)
    expect(result.errors).toContainEqual(
      expect.objectContaining({
        type: 'EMPTY_FLOW',
        message: 'Flow must contain at least one node'
      })
    )
  })

  it('should validate missing trigger', () => {
    const nodes = [
      {
        id: 'node-1',
        type: 'action',
        data: { type: 'search_area', label: '搜索' }
      }
    ]
    const edges = []
    
    const result = validateFlow(nodes, edges)
    
    expect(result.valid).toBe(false)
    expect(result.errors).toContainEqual(
      expect.objectContaining({
        type: 'NO_TRIGGER',
        message: 'Flow must have at least one trigger node'
      })
    )
  })

  it('should validate missing required parameters', () => {
    const nodes = [
      {
        id: 'trigger-1',
        type: 'trigger',
        data: { type: 'mission_start', label: '开始' }
      },
      {
        id: 'action-1',
        type: 'action',
        data: {
          type: 'search_area',
          label: '搜索',
          config: {} // Missing required params
        }
      }
    ]
    
    const result = validateFlow(nodes, [])
    
    expect(result.errors.some(e => 
      e.type === 'MISSING_REQUIRED_PARAM'
    )).toBe(true)
  })

  it('should validate invalid edges', () => {
    const nodes = [
      { id: 'node-1', type: 'trigger', data: { type: 'mission_start' } },
      { id: 'node-2', type: 'action', data: { type: 'search_area' } }
    ]
    
    const edges = [
      { id: 'edge-1', source: 'non-existent-node', target: 'node-2' }
    ]
    
    const result = validateFlow(nodes, edges)
    
    expect(result.errors.some(e => 
      e.type === 'INVALID_EDGE_SOURCE'
    )).toBe(true)
  })

  it('should detect cycles', () => {
    const nodes = [
      { id: 'node-1', type: 'trigger', data: { type: 'mission_start' } },
      { id: 'node-2', type: 'action', data: { type: 'search_area' } },
      { id: 'node-3', type: 'condition', data: { type: 'battery_low' } }
    ]
    
    // Create a cycle: 1 -> 2 -> 3 -> 1
    const edges = [
      { id: 'edge-1', source: 'node-1', target: 'node-2' },
      { id: 'edge-2', source: 'node-2', target: 'node-3' },
      { id: 'edge-3', source: 'node-3', target: 'node-1' }
    ]
    
    const result = validateFlow(nodes, edges)
    
    expect(result.errors.some(e => 
      e.type === 'CIRCULAR_DEPENDENCY'
    )).toBe(true)
  })

  it('should pass valid flow', () => {
    const nodes = [
      {
        id: 'trigger-1',
        type: 'trigger',
        data: { type: 'mission_start', label: '开始' }
      },
      {
        id: 'action-1',
        type: 'action',
        data: {
          type: 'search_area',
          label: '搜索',
          config: {
            area: [{ lat: 40.0, lng: 116.0 }, { lat: 40.1, lng: 116.0 }, { lat: 40.1, lng: 116.1 }],
            altitude: 100,
            speed: 8
          }
        }
      }
    ]
    
    const edges = [
      { id: 'edge-1', source: 'trigger-1', target: 'action-1' }
    ]
    
    const result = validateFlow(nodes, edges)
    
    expect(result.valid).toBe(true)
    expect(result.errors).toHaveLength(0)
  })
})
