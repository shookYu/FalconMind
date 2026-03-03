"""Lightweight SQL injection prevention helpers.

Provides a simple input validator that can be used by API endpoints to
detect common SQL injection patterns in string inputs.
"""

from __future__ import annotations

import re
from typing import Any

_SQL_INJECTION_PATTERNS = [
    r"(;|--|/\*|\*/|DROP\s+TABLE|SELECT\s+.*FROM)",
]
_COMPILED = [re.compile(pat, flags=re.IGNORECASE) for pat in _SQL_INJECTION_PATTERNS]


def contains_sql_injection(value: Any) -> bool:
    if not isinstance(value, str):
        return False
    for pat in _COMPILED:
        if pat.search(value):
            return True
    return False
