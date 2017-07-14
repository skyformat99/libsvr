#include <stdlib.h>
#include <stddef.h>
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "../include/type_def.h"
#include "../include/timer.h"
#include "../include/memory_pool.h"

#pragma comment(lib, "winmm.lib")

bool                g_local_time_run = false;
unsigned            g_local_tick = 0;
volatile time_t     g_local_time = 0;
HANDLE              g_local_time_thread = 0;
long                g_time_zone = 0;

#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

#define INDEX(N) ((mgr->last_tick >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)


struct list_head {
    struct list_head *next, *prev;
};

struct timer_info
{
    struct list_head        entry;
    unsigned                expires;
    unsigned                elapse;
    int                     count;
    void*                   data;
    struct timer_manager*   manager;
};

struct timer_manager 
{
    struct list_head    tv1[TVR_SIZE];
    struct list_head    tv2[TVN_SIZE];
    struct list_head    tv3[TVN_SIZE];
    struct list_head    tv4[TVN_SIZE];
    struct list_head    tv5[TVN_SIZE];

    unsigned            last_tick;
    pfn_on_timer        func_on_timer;
};

static HMEMORYUNIT _get_timer_mgr_unit(void)
{
    static HMEMORYUNIT timer_mgr_unit = 0;

    if (timer_mgr_unit)
    {
        return timer_mgr_unit;
    }

    timer_mgr_unit = create_memory_unit(sizeof(struct timer_manager));

    return timer_mgr_unit;
}

static HMEMORYUNIT _get_timer_info_unit(void)
{
    static HMEMORYUNIT timer_info_unit = 0;

    if (timer_info_unit)
    {
        return timer_info_unit;
    }

    timer_info_unit = create_memory_unit(sizeof(struct timer_info));

    return timer_info_unit;
}

static __inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static __inline void LIST_ADD_TAIL(struct list_head *newone, struct list_head *head)
{
    newone->prev = head->prev;
    head->prev->next = newone;
    newone->next = head;
    head->prev = newone;

}

static __inline void LIST_DEL(struct list_head* prev, struct list_head* next)
{
    next->prev = prev;
    prev->next = next;
}

static __inline void LIST_REPLACE_INIT(struct list_head* old, struct list_head* newone)
{
    newone->next = old->next;
    newone->next->prev = newone;
    newone->prev = old->prev;
    newone->prev->next = newone;

    old->prev = old;
    old->next = old;
}

static __inline int LIST_EMPTY(const struct list_head* head)
{
    return head->next == head;
}

static __inline struct timer_info* _get_info_by_entry(struct list_head* entry_ptr)
{
    return (struct timer_info*)((char*)entry_ptr - offsetof(struct timer_info, entry));
}

static __inline int _is_timer_pend(struct timer_info* info)
{
    return info->entry.next != 0;
}

static __inline void _detach_timer(struct timer_info* info)
{
    LIST_DEL(info->entry.prev, info->entry.next);
    info->entry.next = 0;
}

void _add_timer(struct timer_info* info)
{
    unsigned expires = info->expires;

    unsigned idx = expires - info->manager->last_tick;

    struct list_head* vec;

    int i;

	if (idx < TVR_SIZE)
	{
		i = expires & TVR_MASK;
		vec = info->manager->tv1 + i;
	}
	else if (idx < 1 << (TVR_BITS + TVN_BITS))
	{
		i = (expires >> TVR_BITS) & TVN_MASK;
		vec = info->manager->tv2 + i;
	}
	else if (idx < 1 <<(TVR_BITS + 2*TVN_BITS))
	{
		i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec = info->manager->tv3 + i;
	}
	else if (idx < 1 <<(TVR_BITS + 3*TVN_BITS))
	{
		i = (expires >> (TVR_BITS + 2*TVN_BITS)) & TVN_MASK;
		vec = info->manager->tv4 + i;
	}
	else if ((INT32)idx < 0)
	{
		/*  
		 * Can happen if you add a timer with expires == jiffies,  
		 * or you set a timer to go off in the past  
		 */
		vec = info->manager->tv1 + (info->manager->last_tick & TVR_MASK);
	}
	else
	{
		/* If the timeout is larger than 0xffffffff on 64-bit  
		 * architectures then we use the maximum timeout:  
		 */
		if (idx > 0xffffffffUL)
		{
			idx = 0xffffffffUL;
			expires = idx + info->manager->last_tick;
		}

		i = (expires >> (TVR_BITS + 3*TVN_BITS)) & TVN_MASK;

		vec = info->manager->tv5 + i;
	}

    LIST_ADD_TAIL(&info->entry, vec);
}

unsigned _cascade(struct list_head* vec, unsigned index)
{
    struct timer_info* info;
    struct timer_info* tmp_info;
    struct list_head tv_list;

    LIST_REPLACE_INIT(vec + index, &tv_list);

    for (info = _get_info_by_entry(tv_list.next), tmp_info = _get_info_by_entry(info->entry.next);
        &info->entry != (&tv_list); info = tmp_info, tmp_info = _get_info_by_entry(tmp_info->entry.next))
    {
        _add_timer(info);
    }

    return index;
}

struct timer_manager* create_timer_manager(pfn_on_timer func_on_timer)
{
    int i = 0;

    //struct timer_manager* mgr = (struct timer_manager*)malloc(sizeof(struct timer_manager));

    //mgr->timer_info_pool = create_memory_unit(sizeof(struct timer_info));
    struct timer_manager* mgr = (struct timer_manager*)memory_unit_alloc(_get_timer_mgr_unit(), 32);

    for (i = 0; i < TVN_SIZE; i++)
    {
        INIT_LIST_HEAD(mgr->tv5+i);
        INIT_LIST_HEAD(mgr->tv4+i);
        INIT_LIST_HEAD(mgr->tv3+i);
        INIT_LIST_HEAD(mgr->tv2+i);
    }

    for (i = 0; i < TVR_SIZE; i++)
    {
        INIT_LIST_HEAD(mgr->tv1+i);
    }

    mgr->func_on_timer = func_on_timer;
    mgr->last_tick = GetTickCount();

    return mgr;
}

void destroy_timer_manager(struct timer_manager* mgr)
{
    //if (mgr->timer_info_pool)
    //{
    //    destroy_memory_unit(mgr->timer_info_pool);
    //}

    //free(mgr);
    memory_unit_free(_get_timer_mgr_unit(), mgr);
}

struct timer_info* timer_add(struct timer_manager* mgr, unsigned elapse, int count, void* data)
{
    struct timer_info* info = (struct timer_info*)memory_unit_alloc(_get_timer_info_unit(), 4*1024);

    info->expires = mgr->last_tick + elapse;
    info->elapse = elapse;
    info->count = count;
    info->data = data;
    info->manager = mgr;

    _add_timer(info);

    return info;
}

void timer_mod(struct timer_info* timer, unsigned elapse, int count, void* data)
{
    if (_is_timer_pend(timer))
    {
        _detach_timer(timer);

        timer->expires = timer->manager->last_tick + elapse;
        timer->elapse = elapse;
        timer->count = count;

        if (data)
        {
            timer->data = data;
        }

        _add_timer(timer);
    }
    else
    {
        timer->expires = timer->manager->last_tick + elapse;
        timer->elapse = elapse;
        timer->count = count;

        if (data)
        {
            timer->data = data;
        }
    }
}

void timer_del(struct timer_info* timer)
{
    if (_is_timer_pend(timer))
    {
        _detach_timer(timer);
        timer->data = 0;
        memory_unit_free(_get_timer_info_unit(), timer);
    }
    else
        timer->count = 0;
}

bool timer_update(struct timer_manager* mgr, unsigned elapse)
{
    unsigned tick = GetTickCount();

    bool is_time_out = false;

    struct timer_info* info;

    while (tick != mgr->last_tick)
    {
        struct list_head work_list;
        struct list_head* head = &work_list;

        unsigned index = mgr->last_tick & TVR_MASK;

        if (!index &&
            (!_cascade(mgr->tv2, INDEX(0))) &&
            (!_cascade(mgr->tv3, INDEX(1))) &&
            (!_cascade(mgr->tv4, INDEX(2))))
            _cascade(mgr->tv5, INDEX(3));

        LIST_REPLACE_INIT(mgr->tv1 + index, &work_list);

        while (!LIST_EMPTY(head))
        {
            info = _get_info_by_entry(head->next);

            _detach_timer(info);

            if (info->manager != mgr)
            {
                char* a = NULL;
                *a = 'a';
            }

            if (0 == info->count)
            {
                info->data = 0;
                memory_unit_free(_get_timer_info_unit(), info);
            }
            else
            {
                if (elapse)
                {
                    if (!is_time_out)
                    {
                        if (GetTickCount() - tick > elapse)
                        {
                            is_time_out = true;
                        }
                    }
                }

                if (is_time_out)
                {
                    info->expires = GetTickCount();

                    _add_timer(info);
                }
                else
                {
                    if (info->count > 0)
                    {
                        --info->count;
                    }

                    mgr->func_on_timer(info);

                    if (0 == info->count)
                    {
                        info->data = 0;
                        memory_unit_free(_get_timer_info_unit(), info);
                    }
                    else
                    {
                        info->expires = mgr->last_tick + info->elapse;

                        _add_timer(info);
                    }
                }
            }
        }

        ++mgr->last_tick;

        if (is_time_out)
        {
            return true;
        }
    }

    return false;
}

void* timer_get_data(struct timer_info* timer)
{
    return timer->data;
}

int timer_remain_count(struct timer_info* timer)
{
    return timer->count;
}

bool time_to_string(time_t time, char* str, size_t len)
{
    if (len >= 20)
    {
        struct tm* t_time = localtime(&time);

        if (t_time)
        {
            strftime(str, len, "%Y-%m-%d %H:%M:%S", t_time);
            return true;
        }
    }

    return false;
}

time_t string_to_time(const char* time_string)
{
    struct tm t_time;

    if (6 == sscanf(time_string, "%d-%d-%d %d:%d:%d", 
        &t_time.tm_year, &t_time.tm_mon, &t_time.tm_mday, &t_time.tm_hour, &t_time.tm_min, &t_time.tm_sec))
    {
        t_time.tm_year -= 1900;
        t_time.tm_mon -= 1;
        return mktime(&t_time);
    }

    return 0;
}

unsigned _stdcall local_time_proc(void* param)
{
    unsigned last_tick;
    param;

    while (g_local_time_run)
    {
        last_tick = GetTickCount();

        if (last_tick - g_local_tick > 1000)
        {
            g_local_tick = last_tick;
            g_local_time = time(0);
        }

        Sleep(1);
    }

    return 0;
}

bool init_local_time(void)
{
    unsigned thread_id = 0;
    g_local_tick = GetTickCount();
    g_local_time = time(0);
    g_local_time_run = true;
    _tzset();
    _get_timezone(&g_time_zone);
    g_local_time_thread = (HANDLE)_beginthreadex(0, 0, local_time_proc, 0, 0, &thread_id);

    if (!g_local_time_thread)
    {
        return false;
    }

    Sleep(10);

    return true;
}

void uninit_local_time(void)
{
    g_local_time_run = false;

    WaitForSingleObject(g_local_time_thread, INFINITE);

    if (g_local_time_thread)
    {
        CloseHandle(g_local_time_thread);
        g_local_time_thread = 0;
    }
}

//���ش�1970��1��1��0ʱ0��0�����ھ�����Сʱ��(UTC ʱ��)
time_t now_hour(void)
{
    return g_local_time/3600;
}
//��1970��1��1��0ʱ0��0�����ھ���������(���Ǳ���ʱ��)
time_t now_day(void)
{
    return (g_local_time - g_time_zone)/86400;
}
//��UTC 1970��1��1��0ʱ0��0�����ھ�����������(���Ǳ���ʱ��)
time_t now_week(void)
{
    //1970��1��1����������

    return (now_day()+3)/7;
}
//��UTC 1970��1��1��0ʱ0��0�����ھ���������
unsigned now_month(void)
{
    time_t tt = g_local_time;
    struct tm* pTM = localtime(&tt);

    return (pTM->tm_year-70)*12 + pTM->tm_mon+1;
}
//��UTC 1970��1��1��0ʱ0��0�����ھ���������
unsigned now_year(void)
{
    time_t tt = g_local_time;
    struct tm* pTM = localtime(&tt);

    return pTM->tm_year-70;
}

//��ȡ����ָ���������ӵ�1970��1��1��8ʱ0��0����������(UTCʱ��),ȡֵ����[1, 7]����7���������죬1��������һ
time_t week_day_to_time(time_t week_day)
{
    return week_begin_time(now_week()) + (week_day - 1) * 86400;
}

//����1970��1��1��0ʱ0��0����������(UTCʱ��)ת������,ȡֵ����[1, 7]����7���������죬1��������һ
time_t time_to_week_day(time_t tm)
{
    int week_day = localtime(&tm)->tm_wday;

    return week_day ? week_day : 7;
}

// ��ȡ��N��Ŀ�ʼʱ��time
time_t day_begin_time(time_t day)
{
    return day * 86400 + g_time_zone;
}

// ��ȡ��N�ܵĿ�ʼʱ��time  ���й�ϰ�ߣ�����������һ00:00:00(����ʱ��)��Ϊÿ�ܵĿ�ʼ
time_t week_begin_time(time_t week)
{
    return (week - 1) * 86400 * 7 + 4 * 86400 + g_time_zone;
}

// ��ȡ��N�µĿ�ʼʱ��time  ������ÿ��1��00:00:00(����ʱ��)��Ϊÿ�µĿ�ʼ
time_t month_begin_time(time_t month)
{
    int m = (int)(month - 1) % 12;
    int y = (int)(month - 1) / 12;
    struct tm t = {0, 0, 0, 1, m, y + 70};
    return mktime(&t);
}

// ��ȡ��N��Ŀ�ʼʱ��time  ������ÿ��1��1��00:00:00(����ʱ��)��Ϊÿ�µĿ�ʼ
time_t year_begin_time(time_t year)
{
    struct tm t = {0, 0, 0, 1, 0, (int)year + 70};
    return mktime(&t);
}