#pragma once
#include <time.h>
#ifdef  __cplusplus
extern "C" {
#endif

typedef struct timer_manager* HTIMERMANAGER;
typedef struct timer_info* HTIMERINFO;
typedef void (*pfn_on_timer)(HTIMERINFO timer);

extern bool init_local_time(void);

extern void uninit_local_time(void);

extern HTIMERMANAGER (create_timer_manager)(pfn_on_timer func_on_timer);

extern void (destroy_timer_manager)(HTIMERMANAGER mgr);

extern HTIMERINFO (timer_add)(HTIMERMANAGER mgr, unsigned elapse, int count, void* data);

extern void (timer_mod)(HTIMERINFO timer, unsigned elapse, int count, void* data);

extern void (timer_del)(HTIMERINFO timer);

extern bool (timer_update)(HTIMERMANAGER mgr, unsigned elapse);

extern void* (timer_get_data)(HTIMERINFO timer);

extern int (timer_remain_count)(HTIMERINFO timer);

//////////////////////////////////////////////////////////////////////////
// yyyy-mm-dd hh:mm:ss
extern bool (time_to_string)(time_t time, char* str, size_t len);

extern time_t (string_to_time)(const char* time_string);


extern volatile time_t     g_local_time;
extern long                g_time_zone;


//���شӿ��������ڵĺ�����
#define TIME_ZONE    g_time_zone

//���ش�1970��1��1��0ʱ0��0�����ھ���������(UTC ʱ��)
#define NOW_TIME    g_local_time

//���ش�1970��1��1��0ʱ0��0�����ھ�����Сʱ��(UTC ʱ��)
extern time_t now_hour(void);

//��1970��1��1��0ʱ0��0�����ھ���������(���Ǳ���ʱ��)
extern time_t now_day(void);

//��UTC 1970��1��1��0ʱ0��0�����ھ�����������(���Ǳ���ʱ��)
extern time_t now_week(void);

//��UTC 1970��1��1��0ʱ0��0�����ھ���������
extern unsigned now_month(void);

//��UTC 1970��1��1��0ʱ0��0�����ھ���������
extern unsigned now_year(void);

//��ȡ����ָ���������ӵ�1970��1��1��8ʱ0��0����������(UTCʱ��),ȡֵ����[1, 7]����7���������죬1��������һ
extern time_t week_day_to_time(time_t week_day);

//����1970��1��1��0ʱ0��0����������(UTCʱ��)ת������,ȡֵ����[1, 7]����7���������죬1��������һ
extern time_t time_to_week_day(time_t tm);

// ��ȡ��N��Ŀ�ʼʱ��time
extern time_t day_begin_time(time_t day);

// ��ȡ��N�ܵĿ�ʼʱ��time  ���й�ϰ�ߣ�����������һ00:00:00(����ʱ��)��Ϊÿ�ܵĿ�ʼ
extern time_t week_begin_time(time_t week);

// ��ȡ��N�µĿ�ʼʱ��time  ������ÿ��1��00:00:00(����ʱ��)��Ϊÿ�µĿ�ʼ
extern time_t month_begin_time(time_t month);

// ��ȡ��N��Ŀ�ʼʱ��time  ������ÿ��1��1��00:00:00(����ʱ��)��Ϊÿ�µĿ�ʼ
extern time_t year_begin_time(time_t year);


#ifdef  __cplusplus
}
#endif