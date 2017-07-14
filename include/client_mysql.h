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

typedef MYSQL*      HCLIENTMYSQL;
typedef MYSQL_RES*  HCLIENTMYSQLRES;

typedef struct st_client_mysql_row 
{
    MYSQL_ROW       row_values;
    unsigned long*  row_lengths;
}HCLIENTMYSQLROW;

typedef struct st_client_mysql_value 
{
    char*           value;
    unsigned long   size;
}HCLIENTMYSQLVALUE;

extern HCLIENTMYSQL (create_client_mysql)(const char *host, unsigned int port, const char *user, 
                                          const char *passwd, const char* db, const char* charact_set, 
                                          char* err_info, size_t err_info_size);

extern void (destroy_client_mysql)(HCLIENTMYSQL connection);

extern bool (client_mysql_query)(HCLIENTMYSQL connection, const char* sql, unsigned long length);

extern HCLIENTMYSQLRES (client_mysql_get_result)(HCLIENTMYSQL connection);

extern void (client_mysql_free_result)(HCLIENTMYSQLRES result);

extern HCLIENTMYSQLROW client_mysql_fetch_row(HCLIENTMYSQLRES result);

extern HCLIENTMYSQLROW client_mysql_row(HCLIENTMYSQLRES result, unsigned long long row_index);

extern HCLIENTMYSQLVALUE client_mysql_value(HCLIENTMYSQLROW row, unsigned long field_index);

extern HCLIENTMYSQLVALUE client_mysql_row_field_value(HCLIENTMYSQLRES result, unsigned long long row_index, unsigned long field_index);

extern unsigned char client_mysql_value_uint8(HCLIENTMYSQLVALUE data);

extern char client_mysql_value_int8(HCLIENTMYSQLVALUE data);

extern unsigned short client_mysql_value_uint16(HCLIENTMYSQLVALUE data);

extern short client_mysql_value_int16(HCLIENTMYSQLVALUE data);

extern unsigned int client_mysql_value_uint32(HCLIENTMYSQLVALUE data);

extern int client_mysql_value_int32(HCLIENTMYSQLVALUE data);

extern unsigned long long client_mysql_value_uint64(HCLIENTMYSQLVALUE data);

extern long long client_mysql_value_int64(HCLIENTMYSQLVALUE data);

extern unsigned long client_mysql_escape_string(HCLIENTMYSQL connection, char* src, unsigned long src_size, char* dst, unsigned long dst_size);

#ifdef  __cplusplus
}
#endif