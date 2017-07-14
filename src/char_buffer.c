#include "../include/type_def.h"
#include "../include/memory_pool.h"
#include "../include/char_buffer.h"
#include <memory.h>

#define CRUSH_CODE char* p = 0;*p = 'a';
#define MAX_CHAR_SEGMENT_SIZE   1024

typedef struct st_char_segment 
{
    struct st_char_segment* next;
    size_t  use_size;
    char    data[MAX_CHAR_SEGMENT_SIZE];
}char_segment;

typedef struct st_char_buffer
{
    char_segment* head;     //数据段头
    char_segment* tail;     //数据段尾
    size_t buffer_capacity; //缓存实际大小
    char*  buffer_ptr;      //缓存指针
    size_t buffer_use_size; //缓冲使用大小
    char   default_buffer[MAX_CHAR_SEGMENT_SIZE];
}char_buffer;

static HMEMORYUNIT _get_char_segment_unit(void)
{
    static HMEMORYUNIT char_segment_unit = 0;

    if (char_segment_unit)
    {
        return char_segment_unit;
    }

    char_segment_unit = create_memory_unit(sizeof(char_segment));

    return char_segment_unit;
}

static HMEMORYUNIT _get_char_buffer_unit(void)
{
    static HMEMORYUNIT char_buffer_unit = 0;

    if (char_buffer_unit)
    {
        return char_buffer_unit;
    }

    char_buffer_unit = create_memory_unit(sizeof(char_buffer));

    return char_buffer_unit;
}

char_buffer* create_char_buffer(void)
{
    char_buffer* buffer = (char_buffer*)memory_unit_alloc(_get_char_buffer_unit(), 4096);

    buffer->head = 0;
    buffer->tail = 0;
    buffer->buffer_capacity = MAX_CHAR_SEGMENT_SIZE;
    buffer->buffer_ptr = buffer->default_buffer;
    buffer->buffer_use_size = 0;

    return buffer;
}

void char_buffer_resize(char_buffer* buffer, size_t size)
{
    if (size > buffer->buffer_capacity)
    {
        if (buffer->buffer_ptr == buffer->default_buffer)
        {
            buffer->buffer_ptr = (char*)default_memory_pool_alloc(size);
            memcpy(buffer->buffer_ptr, buffer->default_buffer, buffer->buffer_use_size);
        }
        else
        {
            char* old_ptr = buffer->buffer_ptr;
            buffer->buffer_ptr = (char*)default_memory_pool_alloc(size);
            memcpy(buffer->buffer_ptr, old_ptr, buffer->buffer_use_size);
            default_memory_pool_free(old_ptr);
        }

        buffer->buffer_capacity = memory_unit_size(memory_unit(buffer->buffer_ptr));
    }
}

void char_buffer_append(char_buffer* buffer, const char* data, size_t length)
{
    size_t left;

    if (!buffer->head)
    {
        if (buffer->buffer_use_size + length <= buffer->buffer_capacity)
        {
            memcpy(buffer->buffer_ptr + buffer->buffer_use_size, data, length);
            buffer->buffer_use_size += length;
            return;
        }
        else
        {
            left = buffer->buffer_capacity - buffer->buffer_use_size;
            memcpy(buffer->buffer_ptr + buffer->buffer_use_size, data, left);
            buffer->buffer_use_size += left;
            data += left;
            length -= left;

            buffer->head = (char_segment*)memory_unit_alloc(_get_char_segment_unit(), 4096);
            buffer->tail = buffer->head;
            buffer->head->next = 0;
            buffer->head->use_size = 0;
        }
    }

    if (length + buffer->tail->use_size <= MAX_CHAR_SEGMENT_SIZE)
    {
        memcpy(buffer->tail->data + buffer->tail->use_size, data, length);
        buffer->tail->use_size += length;
        return;
    }
    else
    {
        left = MAX_CHAR_SEGMENT_SIZE - buffer->tail->use_size;
        memcpy(buffer->tail->data + buffer->tail->use_size, data, left);
        buffer->tail->use_size = MAX_CHAR_SEGMENT_SIZE;
        data += left;
        length -= left;
    }

    for (;;)
    {
        char_segment* seg = (char_segment*)memory_unit_alloc(_get_char_segment_unit(), 4096);
        buffer->tail->next = seg;
        buffer->tail = seg;
        seg->next = 0;

        if (length <= MAX_CHAR_SEGMENT_SIZE)
        {
            memcpy(seg->data, data, length);
            seg->use_size = length;
            return;
        }
        else
        {
            memcpy(seg->data, data, MAX_CHAR_SEGMENT_SIZE);
            seg->use_size = MAX_CHAR_SEGMENT_SIZE;
            data += MAX_CHAR_SEGMENT_SIZE;
            length -= MAX_CHAR_SEGMENT_SIZE;
        }
    }
}

const char* char_buffer_c_str(char_buffer* buffer)
{
    size_t total_data_size = buffer->buffer_use_size;

    char_segment* seg = buffer->head;

    while (seg)
    {
        if (seg->next)
        {
            if (seg->use_size != MAX_CHAR_SEGMENT_SIZE)
            {
                CRUSH_CODE;
            }
        }
        total_data_size += seg->use_size;
        seg = seg->next;
    }

    char_buffer_resize(buffer, total_data_size);

    seg = buffer->head;

    while (seg)
    {
        char_segment* del_seg = seg;
        if (seg->next)
        {
            if (seg->use_size != MAX_CHAR_SEGMENT_SIZE)
            {
                CRUSH_CODE;
            }
        }

        memcpy(buffer->buffer_ptr + buffer->buffer_use_size, seg->data, seg->use_size);
        buffer->buffer_use_size += seg->use_size;
        
        seg = seg->next;

        memory_unit_free(_get_char_segment_unit(), del_seg);
    }

    buffer->tail = 0;
    buffer->head = 0;

    return buffer->buffer_ptr;
}

size_t char_buffer_c_str_length(char_buffer* buffer)
{
    if (buffer->head)
    {
        char_buffer_c_str(buffer);
    }

    return buffer->buffer_use_size;
}


void destroy_char_buffer(char_buffer* buffer)
{
    char_segment* seg = buffer->head;

    while (seg)
    {
        char_segment* del_seg = seg;
        seg = seg->next;
        memory_unit_free(_get_char_segment_unit(), del_seg);
    }

    if (buffer->buffer_ptr != buffer->default_buffer)
    {
        default_memory_pool_free(buffer->buffer_ptr);
    }

    memory_unit_free(_get_char_buffer_unit(), buffer);
}