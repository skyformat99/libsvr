#include "../include/utility.h"
#include "../include/rb_tree.h"
#include "../include/memory_pool.h"


ptrdiff_t trace_info_cmp(void* info1, void* info2)
{
    TraceInfo* t1 = (TraceInfo*)info1;
    TraceInfo* t2 = (TraceInfo*)info2;

    if (t1->line < t2->line)
    {
        return -1;
    }
    else if (t1->line > t2->line)
    {
        return 1;
    }

    if (t1->file < t2->file)
    {
        return -1;
    }
    else if (t1->file > t2->file)
    {
        return 1;
    }

    return (t1->name - t2->name);
}

typedef struct st_ptr_info 
{
    TraceInfo*  info;
    size_t      size;
}PtrInfo;

static HRBTREE _get_trace_info_map(void)
{
    static HRBTREE info_tree = 0;

    if (info_tree)
    {
        return info_tree;
    }

    info_tree = create_rb_tree(trace_info_cmp);

    return info_tree;
}

static HRBTREE _get_trace_ptr_map(void)
{
    static HRBTREE ptr_tree = 0;
    
    if (ptr_tree)
    {
        return ptr_tree;
    }

    ptr_tree = create_rb_tree(0);

    return ptr_tree;
}

static HMEMORYUNIT _get_trace_info_unit(void)
{
    static HMEMORYUNIT info_unit = 0;

    if (info_unit)
    {
        return info_unit;
    }

    info_unit = create_memory_unit(sizeof(TraceInfo));

    return info_unit;
}

static HMEMORYUNIT _get_trace_ptr_unit(void)
{
    static HMEMORYUNIT ptr_unit = 0;

    if (ptr_unit)
    {
        return ptr_unit;
    }

    ptr_unit = create_memory_unit(sizeof(PtrInfo));

    return ptr_unit;
}

void trace_alloc(const char* name, const char* file, int line, void* ptr, size_t size)
{
    HRBNODE node;
    TraceInfo info;
    TraceInfo* exist_info;
    PtrInfo* ptr_info;

    info.file = file;
    info.line = line;
    info.name = name;

    node = rb_tree_find_user(_get_trace_info_map(), &info);

    if (node)
    {
        exist_info = (TraceInfo*)rb_node_key_user(node);
        exist_info->size += size;
    }
    else
    {
        exist_info = (TraceInfo*)memory_unit_alloc(_get_trace_info_unit(), 1024);
        exist_info->file = file;
        exist_info->line = line;
        exist_info->name = name;
        exist_info->size = size;

        if (!rb_tree_try_insert_user(_get_trace_info_map(), exist_info, 0, &node))
        {
            char* p = 0;
            *p = 'a';
        }
    }

    ptr_info = (PtrInfo*)memory_unit_alloc(_get_trace_ptr_unit(), 40960);

    ptr_info->info = exist_info;
    ptr_info->size = size;

    if (!rb_tree_try_insert_user(_get_trace_ptr_map(), ptr, ptr_info, &node))
    {
        char* p = 0;
        *p = 'a';
    }
}

void trace_free(void* ptr)
{
    PtrInfo* ptr_info;

    HRBNODE node = rb_tree_find_user(_get_trace_ptr_map(), ptr);

    if (node)
    {
        ptr_info = (PtrInfo*)rb_node_value(node);

        if (ptr_info->info->size < ptr_info->size)
        {
            char* p = 0;
            *p = 'a';
        }

        ptr_info->info->size -= ptr_info->size;

        rb_tree_erase(_get_trace_ptr_map(), node);

        memory_unit_free(_get_trace_ptr_unit(), ptr_info);
    }
    else
    {
        char* p = 0;
        *p = 'a';
    }
}

HRBNODE trace_info_first(void)
{
    return rb_first(_get_trace_info_map());
}

HRBNODE trace_info_next(HRBNODE trace_node)
{
    return rb_next(trace_node);
}

TraceInfo* trace_info_from_node(HRBNODE trace_node)
{
    return (TraceInfo*)rb_node_key_user(trace_node);
}