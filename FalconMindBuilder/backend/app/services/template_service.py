"""
Flow Templates Registry

Manages pre-defined Flow templates for common scenarios.
"""

import json
import os
from typing import List, Dict, Any, Optional
from pathlib import Path

from pydantic import BaseModel


class FlowTemplateMetadata(BaseModel):
    """Flow template metadata"""
    id: str
    name: str
    description: str
    category: str
    version: str
    author: str
    tags: List[str]
    file_path: str


class FlowTemplateRegistry:
    """Registry for Flow templates"""
    
    def __init__(self, templates_dir: str = None):
        if templates_dir is None:
            # Default to the templates directory relative to this file
            current_dir = Path(__file__).parent
            self.templates_dir = current_dir / "flows"
        else:
            self.templates_dir = Path(templates_dir)
        
        self._templates: Dict[str, FlowTemplateMetadata] = {}
        self._load_templates()
    
    def _load_templates(self):
        """Load all templates from the templates directory"""
        if not self.templates_dir.exists():
            return
        
        for template_file in self.templates_dir.glob("*.json"):
            try:
                with open(template_file, 'r', encoding='utf-8') as f:
                    template_data = json.load(f)
                
                # Extract metadata
                metadata = template_data.get("_template", {})
                template_id = template_file.stem
                
                self._templates[template_id] = FlowTemplateMetadata(
                    id=template_id,
                    name=metadata.get("name", template_id),
                    description=metadata.get("description", ""),
                    category=metadata.get("category", "general"),
                    version=metadata.get("version", "1.0"),
                    author=metadata.get("author", "FalconMind"),
                    tags=metadata.get("tags", []),
                    file_path=str(template_file)
                )
            except Exception as e:
                print(f"Error loading template {template_file}: {e}")
    
    def get_all_templates(self) -> List[FlowTemplateMetadata]:
        """Get all available templates"""
        return list(self._templates.values())
    
    def get_templates_by_category(self, category: str) -> List[FlowTemplateMetadata]:
        """Get templates filtered by category"""
        return [
            t for t in self._templates.values()
            if t.category == category
        ]
    
    def get_template(self, template_id: str) -> Optional[Dict[str, Any]]:
        """Get a specific template by ID"""
        if template_id not in self._templates:
            return None
        
        metadata = self._templates[template_id]
        try:
            with open(metadata.file_path, 'r', encoding='utf-8') as f:
                template_data = json.load(f)
            
            # Remove internal metadata and return the actual flow
            template_data.pop("_template", None)
            return template_data
        except Exception as e:
            print(f"Error loading template {template_id}: {e}")
            return None
    
    def get_template_metadata(self, template_id: str) -> Optional[FlowTemplateMetadata]:
        """Get template metadata without loading the full template"""
        return self._templates.get(template_id)
    
    def search_templates(self, query: str) -> List[FlowTemplateMetadata]:
        """Search templates by name, description, or tags"""
        query = query.lower()
        results = []
        
        for template in self._templates.values():
            if (query in template.name.lower() or
                query in template.description.lower() or
                any(query in tag.lower() for tag in template.tags)):
                results.append(template)
        
        return results
    
    def get_categories(self) -> List[str]:
        """Get all template categories"""
        categories = set()
        for template in self._templates.values():
            categories.add(template.category)
        return sorted(list(categories))


# Global registry instance
template_registry = FlowTemplateRegistry()
