#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct mem_pool* HMEMORYPOOL;
typedef struct mem_unit* HMEMORYUNIT;
typedef struct mem_pool_mgr* HMEMORYPOOLMANAGER;

extern HMEMORYPOOL (create_memory_pool)(size_t align, size_t max_mem_size, size_t grow_size);

extern void (destroy_memory_pool)(HMEMORYPOOL pool);

extern void* (memory_pool_alloc)(HMEMORYPOOL pool, size_t mem_size);

extern void* (memory_pool_realloc)(HMEMORYPOOL pool, void* old_mem, size_t mem_size);

extern void (memory_pool_free)(HMEMORYPOOL pool, void* mem);

extern void (memory_pool_set_grow)(HMEMORYPOOL pool, size_t grow_size);

extern size_t (memory_pool_alloc_memory_size)(HMEMORYPOOL pool, void* mem);

extern size_t (memory_pool_use_memory_size)(HMEMORYPOOL pool);

extern HMEMORYUNIT (create_memory_unit)(size_t unit_size);

extern void (destroy_memory_unit)(HMEMORYUNIT unit);

extern void* (memory_unit_alloc)(HMEMORYUNIT unit, size_t grow_byte);

extern void* (memory_unit_alloc_ex)(HMEMORYUNIT unit, size_t grow_number);

extern void (memory_unit_free)(HMEMORYUNIT unit, void* mem);

extern void (memory_unit_quick_free)(HMEMORYUNIT unit, void* mem);

extern size_t (memory_unit_use_memory_size)(HMEMORYUNIT unit);

extern HMEMORYUNIT (memory_unit)(void* mem);

extern size_t (memory_unit_size)(HMEMORYUNIT unit);

extern HMEMORYPOOLMANAGER (create_memory_pool_manager)(void);

extern bool (add_memory_pool_to_manager)(HMEMORYPOOL pool, HMEMORYPOOLMANAGER mgr);

extern void (destroy_memory_pool_manager)(HMEMORYPOOLMANAGER mgr);

extern void* (memory_pool_manager_alloc)(HMEMORYPOOLMANAGER mgr, size_t mem_size);

extern void* (memory_pool_manager_realloc)(HMEMORYPOOLMANAGER mgr, void* old_mem, size_t mem_size);

extern void (memory_pool_manager_free)(HMEMORYPOOLMANAGER mgr, void* mem);

extern void* (default_memory_pool_realloc)(void* old_mem, size_t mem_size);

extern void* (default_memory_pool_alloc)(size_t mem_size);

extern void (default_memory_pool_free)(void* mem);


#ifdef  __cplusplus
}
#endif