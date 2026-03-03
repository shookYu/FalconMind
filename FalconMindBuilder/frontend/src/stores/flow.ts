import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Node, Edge } from '@vue-flow/core'

export interface FlowState {
  nodes: Node[]
  edges: Edge[]
  selectedNode: Node | null
}

export const useFlowStore = defineStore('flow', () => {
  // State
  const nodes = ref<Node[]>([])
  const edges = ref<Edge[]>([])
  const selectedNode = ref<Node | null>(null)
  
  // History for undo/redo
  const history: FlowState[] = []
  const historyIndex = ref(-1)

  // Getters
  const canUndo = computed(() => historyIndex.value > 0)
  const canRedo = computed(() => historyIndex.value < history.length - 1)

  // Actions
  const addNode = (node: Node) => {
    saveState()
    nodes.value.push(node)
  }

  const removeNode = (nodeId: string) => {
    saveState()
    nodes.value = nodes.value.filter(n => n.id !== nodeId)
    edges.value = edges.value.filter(e => e.source !== nodeId && e.target !== nodeId)
  }

  const updateNode = (nodeId: string, data: Partial<Node>) => {
    saveState()
    const node = nodes.value.find(n => n.id === nodeId)
    if (node) {
      Object.assign(node, data)
    }
  }

  const addEdge = (edge: Edge) => {
    saveState()
    edges.value.push(edge)
  }

  const removeEdge = (edgeId: string) => {
    saveState()
    edges.value = edges.value.filter(e => e.id !== edgeId)
  }

  const setSelectedNode = (node: Node | null) => {
    selectedNode.value = node
  }

  const saveState = () => {
    // Remove future states if we're in the middle of history
    if (historyIndex.value < history.length - 1) {
      history.splice(historyIndex.value + 1)
    }
    
    history.push({
      nodes: JSON.parse(JSON.stringify(nodes.value)),
      edges: JSON.parse(JSON.stringify(edges.value)),
      selectedNode: selectedNode.value ? JSON.parse(JSON.stringify(selectedNode.value)) : null
    })
    
    historyIndex.value++
    
    // Limit history size
    if (history.length > 50) {
      history.shift()
      historyIndex.value--
    }
  }

  const undo = () => {
    if (canUndo.value) {
      historyIndex.value--
      const state = history[historyIndex.value]
      nodes.value = JSON.parse(JSON.stringify(state.nodes))
      edges.value = JSON.parse(JSON.stringify(state.edges))
      selectedNode.value = state.selectedNode
    }
  }

  const redo = () => {
    if (canRedo.value) {
      historyIndex.value++
      const state = history[historyIndex.value]
      nodes.value = JSON.parse(JSON.stringify(state.nodes))
      edges.value = JSON.parse(JSON.stringify(state.edges))
      selectedNode.value = state.selectedNode
    }
  }

  const clearFlow = () => {
    saveState()
    nodes.value = []
    edges.value = []
    selectedNode.value = null
  }

  return {
    nodes,
    edges,
    selectedNode,
    canUndo,
    canRedo,
    addNode,
    removeNode,
    updateNode,
    addEdge,
    removeEdge,
    setSelectedNode,
    undo,
    redo,
    clearFlow
  }
})