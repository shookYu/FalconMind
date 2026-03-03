"""Base Pydantic models with input sanitization.

Provides a SafeModel that escapes string inputs to mitigate basic XSS risks
and acts as a central place to extend validation for all API schemas.
"""

from __future__ import annotations

import html
from typing import Any

from pydantic import BaseModel, validator


class SafeModel(BaseModel):
    @validator('*', pre=True)
    def _sanitize_strings(cls, v: Any) -> Any:
        if isinstance(v, str):
            # Basic HTML-escaping to mitigate XSS in string inputs
            return html.escape(v)
        if isinstance(v, list):
            return [cls._sanitize_strings(item) for item in v]
        if isinstance(v, dict):
            return {k: cls._sanitize_strings(val) for k, val in v.items()}
        return v
