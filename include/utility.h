#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#include "rb_tree.h"

#define TRACE_ALLOC(name, ptr, size) trace_alloc(name, __FILE__, __LINE__, ptr, size)
#define TRACE_FREE(ptr) trace_free(ptr);

typedef struct st_trace_info 
{
    const char* name;
    const char* file;
    size_t      line;
    size_t      size;
}TraceInfo;

extern void (trace_alloc)(const char* name, const char* file, int line, void* ptr, size_t size);
extern void (trace_free)(void* ptr);

extern HRBNODE (trace_info_first)(void);
extern HRBNODE (trace_info_next)(HRBNODE trace_node);
extern TraceInfo* (trace_info_from_node)(HRBNODE trace_node);

#ifdef  __cplusplus
}
#endif