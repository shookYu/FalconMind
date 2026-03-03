/**
 * Flow 验证器
 */

export interface ValidationError {
  type: string
  message: string
  nodeId?: string
  edgeId?: string
  field?: string
}

export interface ValidationResult {
  valid: boolean
  errors: ValidationError[]
}

/**
 * 验证 Flow 配置
 */
export function validateFlow(nodes: any[], edges: any[]): ValidationResult {
  const errors: ValidationError[] = []
  
  // 1. 检查空流程
  if (!nodes || nodes.length === 0) {
    errors.push({
      type: 'EMPTY_FLOW',
      message: 'Flow must contain at least one node'
    })
    return { valid: false, errors }
  }
  
  // 2. 检查是否有触发器
  const triggerCount = nodes.filter(n => 
    n.type === 'trigger' || n.data?.type === 'mission_start'
  ).length
  
  if (triggerCount === 0) {
    errors.push({
      type: 'NO_TRIGGER',
      message: 'Flow must have at least one trigger node'
    })
  }
  
  // 3. 验证节点
  const nodeIds = new Set<string>()
  nodes.forEach(node => {
    if (!node.id) {
      errors.push({
        type: 'MISSING_NODE_ID',
        message: 'Node is missing "id" field'
      })
    } else {
      if (nodeIds.has(node.id)) {
        errors.push({
          type: 'DUPLICATE_NODE_ID',
          message: `Duplicate node ID: ${node.id}`,
          nodeId: node.id
        })
      }
      nodeIds.add(node.id)
    }
    
    // 验证节点类型
    if (!node.type) {
      errors.push({
        type: 'MISSING_NODE_TYPE',
        message: 'Node is missing "type" field',
        nodeId: node.id
      })
    }
    
    // 验证搜索区域节点
    if (node.data?.type === 'search_area') {
      const config = node.data.config || {}
      if (!config.area || config.area.length < 3) {
        errors.push({
          type: 'INVALID_AREA',
          message: 'Search area must have at least 3 points',
          nodeId: node.id,
          field: 'area'
        })
      }
      
      if (config.altitude < 10 || config.altitude > 500) {
        errors.push({
          type: 'ALTITUDE_OUT_OF_RANGE',
          message: 'Altitude must be between 10 and 500 meters',
          nodeId: node.id,
          field: 'altitude'
        })
      }
    }
  })
  
  // 4. 验证边
  edges.forEach(edge => {
    if (!edge.source) {
      errors.push({
        type: 'MISSING_EDGE_SOURCE',
        message: 'Edge is missing "source" field',
        edgeId: edge.id
      })
    } else if (!nodeIds.has(edge.source)) {
      errors.push({
        type: 'INVALID_EDGE_SOURCE',
        message: `Edge source node not found: ${edge.source}`,
        edgeId: edge.id
      })
    }
    
    if (!edge.target) {
      errors.push({
        type: 'MISSING_EDGE_TARGET',
        message: 'Edge is missing "target" field',
        edgeId: edge.id
      })
    } else if (!nodeIds.has(edge.target)) {
      errors.push({
        type: 'INVALID_EDGE_TARGET',
        message: `Edge target node not found: ${edge.target}`,
        edgeId: edge.id
      })
    }
  })
  
  // 5. 检测循环依赖
  const cycles = detectCycles(nodes, edges)
  if (cycles.length > 0) {
    cycles.forEach(cycle => {
      errors.push({
        type: 'CIRCULAR_DEPENDENCY',
        message: `Circular dependency detected: ${cycle.join(' -> ')}`
      })
    })
  }
  
  return {
    valid: errors.length === 0,
    errors
  }
}

/**
 * 检测循环依赖
 */
function detectCycles(nodes: any[], edges: any[]): string[][] {
  const adjacency = new Map<string, string[]>()
  
  // 构建邻接表
  nodes.forEach(node => {
    adjacency.set(node.id, [])
  })
  
  edges.forEach(edge => {
    if (adjacency.has(edge.source)) {
      adjacency.get(edge.source)!.push(edge.target)
    }
  })
  
  const cycles: string[][] = []
  const visited = new Set<string>()
  const recStack = new Set<string>()
  const path: string[] = []
  
  function dfs(nodeId: string) {
    visited.add(nodeId)
    recStack.add(nodeId)
    path.push(nodeId)
    
    const neighbors = adjacency.get(nodeId) || []
    for (const neighbor of neighbors) {
      if (!visited.has(neighbor)) {
        dfs(neighbor)
      } else if (recStack.has(neighbor)) {
        // 发现循环
        const cycleStart = path.indexOf(neighbor)
        cycles.push([...path.slice(cycleStart), neighbor])
      }
    }
    
    path.pop()
    recStack.delete(nodeId)
  }
  
  for (const node of nodes) {
    if (!visited.has(node.id)) {
      dfs(node.id)
    }
  }
  
  return cycles
}
