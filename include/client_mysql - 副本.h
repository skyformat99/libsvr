#pragma once

#include <WinSock2.h>

#ifdef x64
#include "../lib3rd/mysql/x64/include/mysql.h"
#include "../lib3rd/mysql/x64/include/errmsg.h"
#else
#include "../lib3rd/mysql/win32/include/mysql.h"
#include "../lib3rd/mysql/win32/include/errmsg.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define CLIENT_MYSQL_RESULT_NEXT_RES    3
#define CLIENT_MYSQL_RESULT_NEXT        2
#define CLIENT_MYSQL_RESULT_END         1
#define CLIENT_MYSQL_RESULT_ERR         0

typedef struct client_mysql* HCLIENTMYSQL;

typedef struct client_mysql_result
{
    MYSQL_RES*          real_mysql_res;
    struct client_mysql* current_mysql;
    unsigned long long  affect_rows;
}CLIENTMYSQLRESULT, *HCLIENTMYSQLRESULT;


extern HCLIENTMYSQL (create_client_mysql)(const char *host, unsigned int port,
    const char *user, const char *passwd, const char* db, const char* charact_set, char* err_info, size_t err_info_size);

extern void (destroy_client_mysql)(HCLIENTMYSQL connection);

extern int (client_mysql_query)(HCLIENTMYSQL connection, const char* sql, unsigned long length, HCLIENTMYSQLRESULT result);

extern MYSQL* (client_mysql_to_mysql)(HCLIENTMYSQL connection);

extern unsigned long (client_mysql_escape_string)(HCLIENTMYSQL connection, char* src, unsigned long src_size, char* dst, unsigned long dst_size);

extern int (client_mysql_next_result)(HCLIENTMYSQLRESULT result);

extern unsigned long long (client_mysql_result_affect)(HCLIENTMYSQLRESULT result);

extern unsigned long (client_mysql_result_value)(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index, char** value);

extern void (client_mysql_free_result)(HCLIENTMYSQLRESULT result);

extern unsigned long long (client_mysql_result_rows)(HCLIENTMYSQLRESULT result);

extern unsigned long (client_mysql_result_fields)(HCLIENTMYSQLRESULT result);

extern unsigned char client_mysql_result_uint8(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern char client_mysql_result_int8(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern unsigned short client_mysql_result_uint16(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern short client_mysql_result_int16(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern unsigned int client_mysql_result_uint32(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern int client_mysql_result_int32(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern unsigned long long client_mysql_result_uint64(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

extern long long client_mysql_result_int64(HCLIENTMYSQLRESULT result, unsigned long long row_index, unsigned long field_index);

#ifdef  __cplusplus
}
#endif