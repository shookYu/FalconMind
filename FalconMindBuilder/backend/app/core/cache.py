"""
Caching Layer for FalconMindBuilder

Provides in-memory caching with TTL support for hot data.
Can be extended to use Redis in production.
"""

import time
import functools
import hashlib
import json
from typing import Any, Optional, Callable, TypeVar
from dataclasses import dataclass
from threading import Lock
import logging

logger = logging.getLogger(__name__)

T = TypeVar('T')


@dataclass
class CacheEntry:
    """Cache entry with TTL"""
    value: Any
    expires_at: float
    access_count: int = 0
    last_accessed: float = 0.0


class Cache:
    """
    In-memory cache with TTL and LRU eviction
    
    Features:
    - TTL support for automatic expiration
    - LRU eviction when max size reached
    - Thread-safe operations
    - Statistics tracking
    """
    
    def __init__(self, max_size: int = 1000, default_ttl: int = 300):
        """
        Initialize cache
        
        Args:
            max_size: Maximum number of entries
            default_ttl: Default TTL in seconds
        """
        self._cache: dict[str, CacheEntry] = {}
        self._max_size = max_size
        self._default_ttl = default_ttl
        self._lock = Lock()
        self._stats = {
            'hits': 0,
            'misses': 0,
            'evictions': 0,
            'expirations': 0
        }
    
    def get(self, key: str) -> Optional[Any]:
        """
        Get value from cache
        
        Args:
            key: Cache key
            
        Returns:
            Cached value or None if not found/expired
        """
        with self._lock:
            entry = self._cache.get(key)
            
            if entry is None:
                self._stats['misses'] += 1
                return None
            
            # Check if expired
            if time.time() > entry.expires_at:
                del self._cache[key]
                self._stats['expirations'] += 1
                self._stats['misses'] += 1
                return None
            
            # Update access stats
            entry.access_count += 1
            entry.last_accessed = time.time()
            self._stats['hits'] += 1
            
            return entry.value
    
    def set(
        self, 
        key: str, 
        value: Any, 
        ttl: Optional[int] = None
    ) -> None:
        """
        Set value in cache
        
        Args:
            key: Cache key
            value: Value to cache
            ttl: TTL in seconds (uses default if not specified)
        """
        with self._lock:
            # Evict expired entries first
            self._evict_expired()
            
            # Evict LRU if at capacity
            if len(self._cache) >= self._max_size and key not in self._cache:
                self._evict_lru()
            
            ttl = ttl or self._default_ttl
            expires_at = time.time() + ttl
            
            self._cache[key] = CacheEntry(
                value=value,
                expires_at=expires_at,
                last_accessed=time.time()
            )
    
    def delete(self, key: str) -> bool:
        """
        Delete entry from cache
        
        Args:
            key: Cache key
            
        Returns:
            True if deleted, False if not found
        """
        with self._lock:
            if key in self._cache:
                del self._cache[key]
                return True
            return False
    
    def clear(self) -> None:
        """Clear all cache entries"""
        with self._lock:
            self._cache.clear()
    
    def get_stats(self) -> dict:
        """Get cache statistics"""
        with self._lock:
            total = self._stats['hits'] + self._stats['misses']
            hit_rate = self._stats['hits'] / total if total > 0 else 0
            
            return {
                'size': len(self._cache),
                'max_size': self._max_size,
                'hits': self._stats['hits'],
                'misses': self._stats['misses'],
                'hit_rate': hit_rate,
                'evictions': self._stats['evictions'],
                'expirations': self._stats['expirations']
            }
    
    def _evict_expired(self) -> None:
        """Remove expired entries"""
        now = time.time()
        expired = [
            key for key, entry in self._cache.items()
            if now > entry.expires_at
        ]
        for key in expired:
            del self._cache[key]
            self._stats['expirations'] += 1
    
    def _evict_lru(self) -> None:
        """Evict least recently used entry"""
        if not self._cache:
            return
        
        lru_key = min(
            self._cache.keys(),
            key=lambda k: self._cache[k].last_accessed
        )
        del self._cache[lru_key]
        self._stats['evictions'] += 1


# Global cache instances
flow_cache = Cache(max_size=100, default_ttl=60)  # 1 minute TTL for flows
project_cache = Cache(max_size=50, default_ttl=120)  # 2 minutes for projects
uav_cache = Cache(max_size=200, default_ttl=30)  # 30 seconds for UAVs (frequent updates)
query_cache = Cache(max_size=500, default_ttl=300)  # 5 minutes for query results


def cached(
    cache_instance: Cache,
    key_prefix: str = "",
    ttl: Optional[int] = None
):
    """
    Decorator to cache function results
    
    Args:
        cache_instance: Cache instance to use
        key_prefix: Prefix for cache key
        ttl: TTL in seconds
    """
    def decorator(func: Callable[..., T]) -> Callable[..., T]:
        @functools.wraps(func)
        def wrapper(*args, **kwargs) -> T:
            # Generate cache key
            key_parts = [key_prefix, func.__name__]
            
            # Add args to key (skip self/cls for methods)
            start_idx = 1 if args and hasattr(args[0], '__class__') else 0
            for arg in args[start_idx:]:
                key_parts.append(str(arg))
            
            # Add kwargs to key (sorted for consistency)
            for k in sorted(kwargs.keys()):
                key_parts.append(f"{k}={kwargs[k]}")
            
            cache_key = hashlib.md5(
                ":".join(key_parts).encode()
            ).hexdigest()
            
            # Try to get from cache
            cached_value = cache_instance.get(cache_key)
            if cached_value is not None:
                logger.debug(f"Cache hit: {func.__name__}")
                return cached_value
            
            # Execute function
            result = func(*args, **kwargs)
            
            # Cache result
            cache_instance.set(cache_key, result, ttl)
            logger.debug(f"Cache miss: {func.__name__}")
            
            return result
        
        return wrapper
    return decorator


def invalidate_cache(cache_instance: Cache, key_pattern: str = ""):
    """
    Invalidate cache entries matching pattern
    
    Args:
        cache_instance: Cache instance
        key_pattern: Pattern to match (empty = clear all)
    """
    if not key_pattern:
        cache_instance.clear()
        logger.info(f"Cleared all cache entries")
    else:
        # Simple pattern matching (could be enhanced with regex)
        with cache_instance._lock:
            keys_to_delete = [
                key for key in cache_instance._cache.keys()
                if key_pattern in key
            ]
            for key in keys_to_delete:
                del cache_instance._cache[key]
        logger.info(f"Invalidated {len(keys_to_delete)} cache entries matching '{key_pattern}'")


# Convenience decorators
flow_cached = functools.partial(cached, flow_cache)
project_cached = functools.partial(cached, project_cache)
uav_cached = functools.partial(cached, uav_cache)
query_cached = functools.partial(cached, query_cache)
