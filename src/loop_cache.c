#include <stdlib.h>
#include <memory.h>
#include "../include/type_def.h"
#include "../include/loop_cache.h"

struct loop_cache 
{
    char*   cache_begin;
    char*   cache_end;
    char*   head;
    char*   tail;
    char*   alloc_cache;
    size_t  size;
};

struct loop_cache* create_loop_cache(size_t size, char* data)
{
    struct loop_cache* cache;

    if (!size)
    {
        return 0;
    }
    
    cache = (struct loop_cache*)malloc(sizeof(struct loop_cache));

    if (data)
    {
        cache->cache_begin = data;
        cache->alloc_cache = 0;
    }
    else
    {
        cache->alloc_cache = (char*)malloc(size);
        cache->cache_begin = cache->alloc_cache;
    }

    cache->head = cache->cache_begin;
    cache->tail = cache->cache_begin;
    cache->cache_end = cache->cache_begin + size;
    cache->size = size;

    return cache;
}

void destroy_loop_cache(struct loop_cache* cache)
{
    if (cache)
    {
        if (cache->alloc_cache)
        {
            free(cache->alloc_cache);
            cache->alloc_cache = 0;
        }

        free(cache);
    }
}

bool loop_cache_push_data(struct loop_cache* cache, const char* data, size_t size)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (size + used + 1 > cache->size)
        return false;

    if (cache->tail + size >= cache->cache_end)
    {
        size_t seg_1 = cache->cache_end - cache->tail;
        size_t seg_2 = size - seg_1;

        memcpy(cache->tail, data, seg_1);
        memcpy(cache->cache_begin, data+seg_1, seg_2);
        cache->tail = cache->cache_begin + seg_2;
    }
    else
    {
        memcpy(cache->tail, data, size);
        cache->tail += size;
    }

    return true;
}

bool loop_cache_pop_data(struct loop_cache* cache, char* data, size_t size)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (size > used)
        return false;

    if (cache->head + size >= cache->cache_end)
    {
        size_t seg_1 = cache->cache_end - cache->head;
        size_t seg_2 = size - seg_1;

        memcpy(data, cache->head, seg_1);
        memcpy(data+seg_1, cache->cache_begin, seg_2);
        cache->head = cache->cache_begin + seg_2;
    }
    else
    {
        memcpy(data, cache->head, size);
        cache->head += size;
    }

    return true;
}

bool loop_cache_copy_data(struct loop_cache* cache, char* data, size_t size)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (size > used)
        return false;

    if (cache->head + size >= cache->cache_end)
    {
        size_t seg_1 = cache->cache_end - cache->head;
        size_t seg_2 = size - seg_1;

        memcpy(data, cache->head, seg_1);
        memcpy(data+seg_1, cache->cache_begin, seg_2);
    }
    else
    {
        memcpy(data, cache->head, size);
    }

    return true;
}

bool loop_cache_push(struct loop_cache* cache, size_t size)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (size + used + 1 > cache->size)
    {
        return false;
    }

    if (cache->tail + size >= cache->cache_end)
    {
        size_t seg_1 = cache->cache_end - cache->tail;
        size_t seg_2 = size - seg_1;
        cache->tail = cache->cache_begin + seg_2;
    }
    else
    {
        cache->tail += size;
    }

    return true;
}

bool loop_cache_pop(struct loop_cache* cache, size_t size)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (size > used)
        return false;

    if (cache->head + size >= cache->cache_end)
    {
        size_t seg_1 = cache->cache_end - cache->head;
        size_t seg_2 = size - seg_1;
        cache->head = cache->cache_begin + seg_2;
    }
    else
    {
        cache->head += size;
    }

    return true;
}

void loop_cache_get_free(struct loop_cache* cache, char** cache_ptr, size_t* cache_len)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (*cache_len)
    {
        if ((*cache_len) > (cache->size - used -1))
        {
            *cache_len = cache->size - used -1;
        }
    }
    else
    {
        *cache_len = cache->size - used -1;
    }

    if (cache->tail + (*cache_len) >= cache->cache_end)
    {
        *cache_len = cache->cache_end - cache->tail;
    }

    *cache_ptr = cache->tail;
}

void loop_cache_get_data(struct loop_cache* cache, char** cache_ptr, size_t* cache_len)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    if (*cache_len)
    {
        if ((*cache_len) > used)
        {
            *cache_len = used;
        }
    }
    else
    {
        *cache_len = used;
    }

    if (cache->head + (*cache_len) >= cache->cache_end)
    {
        *cache_len = cache->cache_end - cache->head;
    }

    *cache_ptr = cache->head;
}

size_t loop_cache_free_size(struct loop_cache* cache)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    return cache->size - used -1;
}

size_t loop_cache_data_size(struct loop_cache* cache)
{
    size_t dist = cache->tail + cache->size - cache->head;
    size_t used = dist >= cache->size ? (dist - cache->size) : dist;

    return used;
}

size_t loop_cache_size(struct loop_cache* cache)
{
    return cache->size;
}

char* loop_cache_get_cache(struct loop_cache* cache)
{
    return cache->cache_begin;
}

void loop_cache_reset(struct loop_cache* cache, size_t size, char* data)
{
    cache->cache_begin = data;

    if (cache->alloc_cache)
    {
        free(cache->alloc_cache);
        cache->alloc_cache = 0;
    }
    

    cache->head = cache->cache_begin;
    cache->tail = cache->cache_begin;
    cache->cache_end = cache->cache_begin + size;
    cache->size = size;
}

void loop_cache_reinit(struct loop_cache* cache)
{
    cache->head = cache->cache_begin;
    cache->tail = cache->cache_begin;
}