#include "../include/type_def.h"
#include "../include/client_mysql.h"
#include "../include/memory_pool.h"
#include <stdio.h>

MYSQL* create_client_mysql(const char *host, unsigned int port, 
    const char *user, const char *passwd, const char* db, const char* charact_set, 
    char* err_info, size_t err_info_size)
{
    unsigned long long character_set_num = 0;

    unsigned long long i = 0;

    my_bool reconnect = true;

    my_bool character_support = false;

    struct st_client_mysql_value value_data;

    char* character_client;
    char* character_connection;
    char* character_result;

    MYSQL_RES* mysql_res_ptr = 0;

    MYSQL* mysql_ptr = mysql_init(0);
    

    if (!mysql_ptr)
    {
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    if (mysql_options(mysql_ptr, MYSQL_OPT_RECONNECT, &reconnect))
    {
        if (err_info)
        {
            const char* err = mysql_error(mysql_ptr);

            size_t err_len = strnlen(err, err_info_size-1);
            memcpy(err_info, err, err_len+1);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    if (!mysql_real_connect(mysql_ptr,host, user, 
        passwd, db, port, 0, CLIENT_MULTI_STATEMENTS))
    {
        if (err_info)
        {
            const char* err = mysql_error(mysql_ptr);

            size_t err_len = strnlen(err, err_info_size-1);
            memcpy(err_info, err, err_len+1);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    if (!charact_set)
    {
        charact_set = "latin1";
    }

    if (!client_mysql_query(mysql_ptr, "SHOW CHARACTER SET", (unsigned long)strlen("SHOW CHARACTER SET")))
    {
        if (err_info)
        {
            const char* err = mysql_error(mysql_ptr);

            size_t err_len = strnlen(err, err_info_size-1);
            memcpy(err_info, err, err_len+1);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    mysql_res_ptr = client_mysql_get_result(mysql_ptr);

    character_set_num = mysql_num_rows(mysql_res_ptr);

    while (i < character_set_num)
    {
        value_data = client_mysql_row_field_value(mysql_res_ptr, i, 0);

        if (!strcmp(charact_set, value_data.value))
        {
            character_support = true;
            break;
        }
        i++;
    }

    client_mysql_free_result(mysql_res_ptr);

    if (!character_support)
    {
        if (err_info)
        {
            sprintf_s(err_info, err_info_size, "character %s not support", charact_set);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    if (mysql_set_character_set(mysql_ptr, charact_set))
    {
        if (err_info)
        {
            const char* err = mysql_error(mysql_ptr);

            size_t err_len = strnlen(err, err_info_size-1);
            memcpy(err_info, err, err_len+1);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    if (!client_mysql_query(mysql_ptr,
        "select @@character_set_client, @@character_set_connection, @@character_set_results;",
        (unsigned long)strlen("select @@character_set_client, @@character_set_connection, @@character_set_results;")))
    {
        if (err_info)
        {
            const char* err = mysql_error(mysql_ptr);

            size_t err_len = strnlen(err, err_info_size-1);
            memcpy(err_info, err, err_len+1);
            err_info[err_info_size-1] = '\0';
        }
        destroy_client_mysql(mysql_ptr);
        return 0;
    }

    mysql_res_ptr = client_mysql_get_result(mysql_ptr);

    value_data = client_mysql_row_field_value(mysql_res_ptr, 0, 0);
    character_client = value_data.value;

    value_data = client_mysql_row_field_value(mysql_res_ptr, 0, 1);
    character_connection = value_data.value;

    value_data = client_mysql_row_field_value(mysql_res_ptr, 0, 2);
    character_result = value_data.value;

    if (!strcmp(character_client, character_connection))
    {
        if (!strcmp(character_connection, character_result))
        {
            client_mysql_free_result(mysql_res_ptr);
            return mysql_ptr;
        }
    }

    client_mysql_free_result(mysql_res_ptr);

    if (err_info)
    {
        sprintf_s(err_info, err_info_size, "character_client: %s character_connection: %s character_result: %s",
            character_client, character_connection, character_result);
    }

    destroy_client_mysql(mysql_ptr);

    return 0;
}

void destroy_client_mysql(MYSQL* connection)
{
    if (connection)
    {
        mysql_close(connection);
    }
}

bool client_mysql_query(MYSQL* connection, const char* sql, unsigned long length)
{
    int query_ret;
    int try_query_count = 0;

QUERY:

    if (length)
    {
        query_ret = mysql_real_query(connection, sql, length);
    }
    else
    {
        query_ret = mysql_query(connection, sql);
    }

    if (query_ret)
    {
        switch (mysql_errno(connection))
        {
        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
            {
                if (try_query_count < 3)
                {
                    try_query_count++;
                    mysql_ping(connection);
                    goto QUERY;
                }
                else
                {
                    return false;
                }
            }
            break;
        }
    }

    return true;
}

MYSQL_RES* client_mysql_get_result(MYSQL* connection)
{
    return mysql_store_result(connection);
}

void client_mysql_free_result(MYSQL_RES* result)
{
    mysql_free_result(result);
}

struct st_client_mysql_row client_mysql_fetch_row(MYSQL_RES* result)
{
    struct st_client_mysql_row row;

    row.row_values = mysql_fetch_row(result);
    if (row.row_values)
    {
        row.row_lengths = mysql_fetch_lengths(result);
    }
    else
    {
        row.row_lengths = 0;
    }

    return row;
}

struct st_client_mysql_row client_mysql_row(MYSQL_RES* result, unsigned long long row_index)
{
    mysql_data_seek(result, row_index);

    return client_mysql_fetch_row(result);
}

struct st_client_mysql_value client_mysql_value(struct st_client_mysql_row row, unsigned long field_index)
{
    struct st_client_mysql_value data;

    data.value = row.row_values[field_index];
    data.size = row.row_lengths[field_index];

    return data;
}

struct st_client_mysql_value client_mysql_row_field_value(MYSQL_RES* result, unsigned long long row_index, unsigned long field_index)
{
    struct st_client_mysql_value data;

    MYSQL_ROW row_values = 0;
    unsigned long* row_lengths = 0;

    mysql_data_seek(result, row_index);

    row_values = mysql_fetch_row(result);

    if (row_values)
    {
        row_lengths = mysql_fetch_lengths(result);

        data.value = row_values[field_index];
        data.size = row_lengths[field_index];
    }
    else
    {
        data.value = 0;
        data.size = 0;
    }

    return data;
}

unsigned long client_mysql_escape_string(MYSQL* connection, char* src, unsigned long src_size, char* dst, unsigned long dst_size)
{
    if (dst_size < src_size*2+1)
    {
        return (unsigned long)-1;
    }
    return mysql_real_escape_string(connection, dst, src, src_size);
}

unsigned char client_mysql_value_uint8(struct st_client_mysql_value data)
{
    unsigned char value_uint8;

    value_uint8 = (unsigned char)atoi(data.value);

    return value_uint8;
}

char client_mysql_value_int8(struct st_client_mysql_value data)
{
    char value_int8;

    value_int8 = (char)atoi(data.value);

    return value_int8;
}

unsigned short client_mysql_value_uint16(struct st_client_mysql_value data)
{
    unsigned short value_uint16;

    value_uint16 = (unsigned short)atoi(data.value);

    return value_uint16;
}

short client_mysql_value_int16(struct st_client_mysql_value data)
{
    short value_int16;

    value_int16 = (short)atoi(data.value);

    return value_int16;
}

unsigned int client_mysql_value_uint32(struct st_client_mysql_value data)
{
    unsigned int value_uint32;

    value_uint32 = (unsigned int)_strtoui64(data.value, 0, 10);

    return value_uint32;
}

int client_mysql_value_int32(struct st_client_mysql_value data)
{
    int value_int32;

    value_int32 = atoi(data.value);

    return value_int32;
}

unsigned long long client_mysql_value_uint64(struct st_client_mysql_value data)
{
    unsigned long long value_uint64;

    value_uint64 = _strtoui64(data.value, 0, 10);

    return value_uint64;
}

long long client_mysql_value_int64(struct st_client_mysql_value data)
{
    long long value_int64;

    value_int64 = _strtoi64(data.value, 0, 10);

    return value_int64;
}