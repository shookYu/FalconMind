import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { blocksApi } from '@/api/blocks';
import type { Block, BlockCategory, BlockCreate } from '@/types/block';

export const useBlocksStore = defineStore('blocks', () => {
  // State
  const categories = ref<BlockCategory[]>([]);
  const blocks = ref<Block[]>([]);
  const currentBlock = ref<Block | null>(null);
  const loading = ref(false);
  const error = ref<string | null>(null);

  // Getters
  const blocksByCategory = computed(() => {
    const grouped: Record<string, Block[]> = {};
    
    categories.value.forEach(cat => {
      grouped[cat.id] = blocks.value.filter(b => b.category_id === cat.id);
    });
    
    return grouped;
  });

  const getBlockById = computed(() => {
    return (id: string) => blocks.value.find(b => b.id === id) || null;
  });

  // Actions
  const fetchCategories = async () => {
    loading.value = true;
    
    try {
      categories.value = await blocksApi.getCategories();
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch categories';
    } finally {
      loading.value = false;
    }
  };

  const fetchBlocks = async (params?: { category?: string; search?: string }) => {
    loading.value = true;
    
    try {
      blocks.value = await blocksApi.getBlocks(params);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch blocks';
    } finally {
      loading.value = false;
    }
  };

  const fetchBlock = async (id: string) => {
    loading.value = true;
    
    try {
      currentBlock.value = await blocksApi.getBlock(id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch block';
    } finally {
      loading.value = false;
    }
  };

  const createBlock = async (data: BlockCreate) => {
    loading.value = true;
    
    try {
      const block = await blocksApi.createBlock(data);
      blocks.value.push(block);
      return block;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to create block';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateBlock = async (id: string, data: Partial<BlockCreate>) => {
    loading.value = true;
    
    try {
      const block = await blocksApi.updateBlock(id, data);
      const index = blocks.value.findIndex(b => b.id === id);
      if (index !== -1) {
        blocks.value[index] = block;
      }
      return block;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update block';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const deleteBlock = async (id: string) => {
    loading.value = true;
    
    try {
      await blocksApi.deleteBlock(id);
      blocks.value = blocks.value.filter(b => b.id !== id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to delete block';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  return {
    categories,
    blocks,
    currentBlock,
    loading,
    error,
    blocksByCategory,
    getBlockById,
    fetchCategories,
    fetchBlocks,
    fetchBlock,
    createBlock,
    updateBlock,
    deleteBlock,
  };
});
