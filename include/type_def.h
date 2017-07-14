#pragma once
#include <crtdefs.h>

typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef signed long long    INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned long long  UINT64, *PUINT64;

#ifndef __cplusplus
#define bool unsigned char
#define true 1
#define false 0
#endif

typedef ptrdiff_t (*user_key_cmp)(void*, void*);
typedef unsigned (_stdcall *user_thread_proc)(void*);

