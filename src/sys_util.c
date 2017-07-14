#include <Windows.h>
#include <process.h>
#include "../include/type_def.h"
#include "../include/memory_pool.h"
#include "../include/sys_util.h"

struct st_sthread 
{
    HANDLE              thread_h;
    unsigned            thread_id;
    user_thread_proc    thread_proc;
    void*               thread_param;
};

static HMEMORYUNIT _get_sthread_unit(void)
{
    static HMEMORYUNIT sthread_unit = 0;

    if (sthread_unit)
    {
        return sthread_unit;
    }

    sthread_unit = create_memory_unit(sizeof(struct st_sthread));

    return sthread_unit;
}

struct st_sthread* create_sthread(user_thread_proc thread_proc, void* param, unsigned stack_size, bool is_suspend)
{
    struct st_sthread* sthread;
    unsigned flag = 0;
    sthread = (struct st_sthread*)memory_unit_alloc(_get_sthread_unit(), 1024);

    if (is_suspend)
    {
        flag = CREATE_SUSPENDED;
    }

    sthread->thread_h = (HANDLE)_beginthreadex(0, stack_size, thread_proc, param, flag, &(sthread->thread_id));

    if (!sthread->thread_h)
    {
        memory_unit_free(_get_sthread_unit(), sthread);
        return 0;
    }

    return sthread;
}

void wait_sthread(struct st_sthread* sthread, unsigned wait_time)
{
    if (sthread->thread_h)
    {
        WaitForSingleObject(sthread->thread_h, wait_time);
    }
}

void destroy_sthread(struct st_sthread* sthread)
{
    if (sthread->thread_h)
    {
        CloseHandle(sthread->thread_h);
    }

    memory_unit_free(_get_sthread_unit(), sthread);
}