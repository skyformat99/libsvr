#include <stdlib.h>
#include <memory.h>

#include "../include/type_def.h"
#include "../include/rb_tree.h"
#include "../include/memory_pool.h"

#define CRUSH_CODE char* p = 0;*p = 'a';

struct mem_block 
{
    struct mem_block* next; //指向下一个内存块的
};

struct mem_unit 
{
    size_t              unit_size;      //内存单元的大小
    struct mem_block*   block_head;     //内存块链表头
    void*               unit_free_head; //可分配内存单元链表头
    size_t              use_mem_size;   //实际占用内存数量         
};

struct mem_pool 
{
    struct mem_unit**   units;          //内存池数组
    size_t              unit_size;      //内存池数组长度
    size_t              shift;          //位移偏移量
    size_t              align;          //内存池对齐字节数,必须是4的倍数
    size_t              grow;           //每次扩展内存大小
    size_t              max_mem_size;   //内存池管理的最大内存大小，大于此大小的内存由系统托管
    size_t              use_mem_size;   //实际占用内存数量
};

struct mem_block* _create_memory_block(struct mem_unit* unit, size_t unit_count)
{
    unsigned char* ptr;
    size_t i;
    struct mem_block* block;
    size_t block_size = sizeof(struct mem_block) + unit_count*(sizeof(void*)+unit->unit_size);

    block = (struct mem_block*)malloc(block_size);

    if (block)
    {
        block->next = unit->block_head;
        unit->block_head = block;

        ptr = (unsigned char*)block + sizeof(struct mem_block);

        for (i = 0; i < unit_count-1; i++)
        {
            *((void**)ptr) = ptr + sizeof(void*)+unit->unit_size;
            ptr += sizeof(void*)+unit->unit_size;
        }

        *((void**)ptr) = unit->unit_free_head;
        unit->unit_free_head = (unsigned char*)block + sizeof(struct mem_block);
        unit->use_mem_size += block_size;
    }

    return block;
}

struct mem_unit* create_memory_unit(size_t unit_size)
{
    struct mem_unit* unit = (struct mem_unit*)malloc(sizeof(struct mem_unit));
    if (unit)
    {
        unit->unit_size = unit_size;
        unit->block_head = 0;
        unit->unit_free_head = 0;
        unit->use_mem_size = 0;
    }

    return unit;
}

void destroy_memory_unit(struct mem_unit* unit)
{
    while (unit->block_head)
    {
        struct mem_block* block = unit->block_head;
        unit->block_head = block->next;

        free(block);
    }

    free(unit);
}

struct mem_pool* create_memory_pool(size_t align /*= 16*/, size_t max_mem_size /*= 4194304*/, size_t grow_size)
{
    struct mem_pool* pool = (struct mem_pool*)malloc(sizeof(struct mem_pool));

    size_t k = align, i = 0, j = 0, unit_arry_size;

    if (!align || (align & 3))   // 4 align
        return 0;

    if (!max_mem_size || (max_mem_size & (align - 1)))    // MAX_MEM_ALIGN align
        return 0;

    for (; !(k & 1); k >>= 1, ++i);             // mod MAX_MEM_ALIGN align

    pool->use_mem_size = 0;
    pool->shift = i;
    pool->unit_size = (max_mem_size >> i);

    unit_arry_size = pool->unit_size*sizeof(struct mem_unit*);
    pool->units = (struct mem_unit**)malloc(unit_arry_size);
    pool->use_mem_size += unit_arry_size;

    for (j = 0; j < pool->unit_size; j++)
    {
        pool->units[j] = 0;
    }

    pool->align = align;
    pool->grow = 4*1024;
    if (grow_size > pool->grow)
    {
        pool->grow = grow_size;
    }
    pool->max_mem_size = max_mem_size;

    return pool;
}

void destroy_memory_pool(struct mem_pool* pool)
{
    size_t i;

    for (i = 0; i < pool->unit_size; ++i)
    {
        if (pool->units[i])
        {
            destroy_memory_unit(pool->units[i]);
        }
    }

    free(pool->units);

    free(pool);
}

void* memory_unit_alloc(struct mem_unit* unit, size_t grow_size)
{
    void* alloc_mem;

    if (!unit->unit_free_head)
    {
        size_t unit_count;

        if (!grow_size)
        {
            return 0;
        }

        if (grow_size <= sizeof(struct mem_block))
        {
            grow_size = sizeof(struct mem_block);
        }
        
        unit_count = (grow_size - sizeof(struct mem_block))/(sizeof(void*)+unit->unit_size);

        if (unit_count <= 0)
        {
            unit_count = 1;
        }

        if (!_create_memory_block(unit, unit_count))
        {
            return 0;
        }
    }

    alloc_mem = unit->unit_free_head;

    unit->unit_free_head = *(void**)alloc_mem;

    *(void**)alloc_mem = unit;

    return (unsigned char*)alloc_mem + sizeof(void*);
}

void* memory_unit_alloc_ex(struct mem_unit* unit, size_t grow_count)
{
    void* alloc_mem;

    if (!unit->unit_free_head)
    {
        if (!grow_count)
        {
            return 0;
        }

        if (!_create_memory_block(unit, grow_count))
        {
            return 0;
        }
    }

    alloc_mem = unit->unit_free_head;

    unit->unit_free_head = *(void**)alloc_mem;

    *(void**)alloc_mem = unit;

    return (unsigned char*)alloc_mem + sizeof(void*);
}

void memory_unit_free(struct mem_unit* unit, void* mem)
{
    struct mem_unit* check_unit = *(struct mem_unit**)((unsigned char*)mem - sizeof(void*));

    if (unit)
    {
        if (check_unit != unit)
        {
            return;
        }
    }

    *(void**)((unsigned char*)mem - sizeof(void*)) = check_unit->unit_free_head;
    check_unit->unit_free_head = (unsigned char*)mem - sizeof(void*);
}

void memory_unit_quick_free(struct mem_unit* unit, void* mem)
{
    *(void**)((unsigned char*)mem - sizeof(void*)) = unit->unit_free_head;
    unit->unit_free_head = (unsigned char*)mem - sizeof(void*);
}

void* memory_pool_alloc(struct mem_pool* pool, size_t mem_size)
{
    size_t i;
    struct mem_unit* unit;
    void* alloc_mem;

    if (!mem_size)
    {
        return 0;
    }

    if (mem_size > pool->max_mem_size)
    {
        unsigned char* mem = (unsigned char*)malloc(sizeof(void*) + sizeof(size_t) + mem_size);
        *(size_t*)mem = mem_size;
        *(void**)(mem + sizeof(size_t)) = 0;
        return mem + sizeof(void**) + sizeof(size_t);
    }

    i = (mem_size & (pool->align -1)) ?
        (mem_size >> pool->shift) : (mem_size >> pool->shift) - 1;

    unit = pool->units[i];

    if (!unit)
    {
        pool->units[i] = create_memory_unit((i + 1) * pool->align);
        unit = pool->units[i];

        if (!unit)
        {
            return 0;
        }
        pool->use_mem_size += sizeof(struct mem_unit);
    }

    if (!unit->unit_free_head)
    {
        size_t last_unit_size = unit->use_mem_size;
        size_t unit_count = (pool->grow - sizeof(struct mem_block))/(sizeof(void*)+unit->unit_size);
        if (unit_count <= 0)
        {
            unit_count = 1;
        }

        if (!_create_memory_block(unit, unit_count))
        {
            return 0;
        }

        pool->use_mem_size += (unit->use_mem_size - last_unit_size);
    }

    alloc_mem = unit->unit_free_head;

    unit->unit_free_head = *(void**)alloc_mem;

    *(void**)alloc_mem = unit;

    return (unsigned char*)alloc_mem + sizeof(void*);
}

void* memory_pool_realloc(struct mem_pool* pool, void* old_mem, size_t mem_size)
{
    void* new_mem = 0;
    struct mem_unit* unit;

    if (!old_mem)
    {
        return memory_pool_alloc(pool, mem_size);
    }

    unit = *(struct mem_unit**)((unsigned char*)old_mem - sizeof(void*));

    if (!unit)
    {
        if (*(size_t*)((unsigned char*)old_mem-sizeof(void*)-sizeof(size_t)) <= pool->max_mem_size)
        {
            return 0;
        }

        if (mem_size <= *(size_t*)((unsigned char*)old_mem-sizeof(void*)-sizeof(size_t)))
            return old_mem;

        new_mem = memory_pool_alloc(pool, mem_size);
        if (!new_mem)
        {
            return new_mem;
        }

        memcpy(new_mem, old_mem, *(size_t*)((unsigned char*)old_mem-sizeof(void*)-sizeof(size_t)));
        free((unsigned char*)old_mem-sizeof(void*)-sizeof(size_t));
        return new_mem;
    }
    else
    {
        size_t i = (unit->unit_size & (pool->align -1)) ?
            (unit->unit_size >> pool->shift) : (unit->unit_size >> pool->shift) - 1;

        if (unit != pool->units[i])
        {
            return 0;
        }

        if (unit->unit_size >= mem_size)
            return old_mem;

        new_mem = memory_pool_alloc(pool, mem_size);

        memcpy(new_mem, old_mem, unit->unit_size);

        *(void**)((unsigned char*)old_mem - sizeof(void*)) = unit->unit_free_head;
        unit->unit_free_head = (unsigned char*)old_mem - sizeof(void*);

        return new_mem;
    }
}

void memory_pool_free(struct mem_pool* pool, void* mem)
{
    struct mem_unit* unit;
    size_t i;

    if (!mem)
        return;

    unit = *(struct mem_unit**)((unsigned char*)mem - sizeof(void*));

    if (!unit)
    {
        if (*(size_t*)((unsigned char*)mem -sizeof(void*)-sizeof(size_t)) > pool->max_mem_size)
        {
            free((unsigned char*)mem - sizeof(void*)-sizeof(size_t));
        }
        return;
    }

    i = (unit->unit_size & (pool->align -1)) ?
        (unit->unit_size >> pool->shift) : (unit->unit_size >> pool->shift) - 1;

    if (unit != pool->units[i])
    {
        return;
    }

    *(void**)((unsigned char*)mem - sizeof(void*)) = unit->unit_free_head;
    unit->unit_free_head = (unsigned char*)mem - sizeof(void*);
}

void memory_pool_set_grow(struct mem_pool* pool, size_t grow_size)
{
    if (grow_size <= 4*1024)
    {
        pool->grow = 4*1024;
    }
    else
        pool->grow = grow_size;
}

size_t memory_pool_alloc_memory_size(struct mem_pool* pool, void* mem)
{
    struct mem_unit* unit;
    size_t i;

    if (!mem)
        return 0;

    unit = *(struct mem_unit**)((unsigned char*)mem - sizeof(void*));

    if (!unit)
    {
        if (*(size_t*)((unsigned char*)mem -sizeof(void*)-sizeof(size_t)) > pool->max_mem_size)
        {
            return *(size_t*)((unsigned char*)mem -sizeof(void*)-sizeof(size_t));
        }
        return 0;
    }

    i = (unit->unit_size & (pool->align -1)) ?
        (unit->unit_size >> pool->shift) : (unit->unit_size >> pool->shift) - 1;

    if (unit != pool->units[i])
    {
        return 0;
    }

    return unit->unit_size;
}

size_t memory_pool_use_memory_size(struct mem_pool* pool)
{
    return pool->use_mem_size;
}

size_t memory_unit_use_memory_size(struct mem_unit* unit)
{
    return unit->use_mem_size;
}

struct mem_unit* memory_unit(void* mem)
{
    return *(struct mem_unit**)((unsigned char*)mem - sizeof(void*));
}

size_t memory_unit_size(struct mem_unit* unit)
{
    return unit->unit_size;
}

struct mem_pool_mgr
{
    HRBTREE pool_map;
};

struct mem_pool_mgr* create_memory_pool_manager(void)
{
    struct mem_pool_mgr* pool_mgr = (struct mem_pool_mgr*)malloc(sizeof(struct  mem_pool_mgr));
    pool_mgr->pool_map = create_rb_tree(0);

    return pool_mgr;
}

bool add_memory_pool_to_manager(struct mem_pool* pool, struct mem_pool_mgr* mgr)
{
    HRBNODE tmp_node = 0;

    return rb_tree_try_insert_int(mgr->pool_map, pool->max_mem_size, pool, &tmp_node);
}

void destroy_memory_pool_manager(struct mem_pool_mgr* mgr)
{
    HRBNODE pool_node = rb_first(mgr->pool_map);
    while (pool_node)
    {
        destroy_memory_pool((HMEMORYPOOL)rb_node_value(pool_node));
        pool_node = rb_next(pool_node);
    }

    destroy_rb_tree(mgr->pool_map);
    free(mgr);
}

extern HRBNODE rb_tree_find_int_near_large(HRBTREE tree, size_t key);

void* memory_pool_manager_alloc(struct mem_pool_mgr* mgr, size_t mem_size)
{
    HRBNODE node = rb_tree_find_int_near_large(mgr->pool_map, mem_size);

    if (node)
    {
        return memory_pool_alloc((HMEMORYPOOL)rb_node_value(node), mem_size);
    }

    return 0;
}

void* memory_pool_manager_realloc(struct mem_pool_mgr* mgr, void* old_mem, size_t mem_size)
{
    HMEMORYUNIT unit = 0;
    void* new_mem = 0;
    HRBNODE old_node = 0;
    HRBNODE new_node = 0;

    if (!old_mem)
    {
        return memory_pool_manager_alloc(mgr, mem_size);
    }

    unit = memory_unit(old_mem);

    if (!unit)
    {
        return memory_pool_realloc((HMEMORYPOOL)rb_node_value(rb_last(mgr->pool_map)), old_mem, mem_size);
    }

    if (mem_size <= unit->unit_size)
    {
        return old_mem;
    }

    old_node = rb_tree_find_int_near_large(mgr->pool_map, unit->unit_size);
    new_node = rb_tree_find_int_near_large(mgr->pool_map, mem_size);

    if (old_node == new_node)
    {
        return memory_pool_realloc((HMEMORYPOOL)rb_node_value(old_node), old_mem, mem_size);
    }
    else
    {
        new_mem = memory_pool_alloc((HMEMORYPOOL)rb_node_value(new_node), mem_size);
        memcpy(new_mem, old_mem, unit->unit_size);
        memory_pool_free((HMEMORYPOOL)rb_node_value(old_node), old_mem);
        return new_mem;
    }
}

void memory_pool_manager_free(struct mem_pool_mgr* mgr, void* mem)
{
    HMEMORYUNIT unit = 0;
    HRBNODE pool_node = 0;
    HMEMORYPOOL pool = 0;

    if (!mem)
        return;
    unit = memory_unit(mem);

    if (!unit)
    {
        memory_pool_free((HMEMORYPOOL)rb_node_value(rb_last(mgr->pool_map)), mem);
        return;
    }

    pool_node = rb_tree_find_int_near_large(mgr->pool_map, unit->unit_size);

    if (!pool_node)
    {
        CRUSH_CODE;
    }

    pool = (HMEMORYPOOL)rb_node_value(pool_node);

    if (pool->units[unit->unit_size/pool->align - 1] == unit)
    {
        memory_unit_quick_free(unit, mem);
    }
    else
    {
        CRUSH_CODE;
    }
}

static struct mem_pool_mgr* g_def_mem_pool_mgr = 0;

void create_default_memory_pool_manager(void)
{
    size_t align, max_size, grow_size;
    g_def_mem_pool_mgr = (struct mem_pool_mgr*)malloc(sizeof(struct  mem_pool_mgr));
    g_def_mem_pool_mgr->pool_map = create_rb_tree(0);

    align = 8;
    max_size = 128;
    grow_size = 4*1024;

    while (max_size <= 65536)
    {
        HMEMORYPOOL pool = create_memory_pool(align, max_size, grow_size);

        rb_tree_insert_int(g_def_mem_pool_mgr->pool_map, pool->max_mem_size, pool);
        align *= 2;
        max_size *= 2;
    }
}

void destroy_default_memory_pool_manager(void)
{
    destroy_memory_pool_manager(g_def_mem_pool_mgr);
}

void* default_memory_pool_realloc(void* old_mem, size_t mem_size)
{
    return memory_pool_manager_realloc(g_def_mem_pool_mgr, old_mem, mem_size);
}

void* default_memory_pool_alloc(size_t mem_size)
{
    return memory_pool_manager_alloc(g_def_mem_pool_mgr, mem_size);
}

void default_memory_pool_free(void* mem)
{
    memory_pool_manager_free(g_def_mem_pool_mgr, mem);
}
