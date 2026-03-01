export interface BlockParameter {
  name: string;
  type: string;
  default: unknown;
  description: string;
  required: boolean;
  options?: unknown[];
}

export interface BlockInput {
  name: string;
  type: string;
  description: string;
  required: boolean;
}

export interface BlockOutput {
  name: string;
  type: string;
  description: string;
}

export interface BlockCategory {
  id: string;
  name: string;
  icon: string;
  color: string;
  block_count: number;
}

export interface Block {
  id: string;
  name: string;
  description: string;
  category_id: string;
  icon: string;
  color: string;
  inputs: BlockInput[];
  outputs: BlockOutput[];
  parameters: BlockParameter[];
  code_template?: string;
  created_at?: string;
  updated_at?: string;
}

export interface BlockCreate {
  id: string;
  name: string;
  description: string;
  category_id: string;
  icon?: string;
  color?: string;
  inputs?: BlockInput[];
  outputs?: BlockOutput[];
  parameters?: BlockParameter[];
  code_template?: string;
}
