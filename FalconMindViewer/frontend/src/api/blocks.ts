import { api } from './client';
import type { Block, BlockCategory, BlockCreate } from '@/types/block';

export const blocksApi = {
  // Get all categories
  getCategories: () => 
    api.get<BlockCategory[]>('/blocks/categories'),
  
  // Get blocks by category
  getBlocksByCategory: (categoryId: string) => 
    api.get<Block[]>(`/blocks/categories/${categoryId}/blocks`),
  
  // Get all blocks with optional filters
  getBlocks: (params?: { category?: string; search?: string }) => 
    api.get<Block[]>('/blocks', { params }),
  
  // Get single block
  getBlock: (id: string) => 
    api.get<Block>(`/blocks/${id}`),
  
  // Create block (admin only)
  createBlock: (data: BlockCreate) => 
    api.post<Block>('/blocks', data),
  
  // Update block (admin only)
  updateBlock: (id: string, data: Partial<BlockCreate>) => 
    api.put<Block>(`/blocks/${id}`, data),
  
  // Delete block (admin only)
  deleteBlock: (id: string) => 
    api.delete(`/blocks/${id}`),
};
