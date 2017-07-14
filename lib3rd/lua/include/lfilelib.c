#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <string.h>

#define FILE_HANDLE "__file"

static int file_open(lua_State* L)
{
    FILE* file = NULL;
    int err = 0;
    char* file_name = NULL;
    char* open_mode = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);
    file_name = luaL_checkstring(L, 2);
    open_mode = luaL_checkstring(L, 3);

    lua_pushstring(L, FILE_HANDLE);
    lua_gettable(L, 1);
    if (LUA_TNIL != lua_type(L, -1))
    {
        lua_pop(L, 1);
        luaL_error(L, "arg #1 is bad, file has been opened.");
    }

    file = fopen(file_name, open_mode);
    if (!file)
    {
        _get_errno(&err);
        luaL_error(L, "open file %s fail, mode = %s, errno = %d.", 
            file_name, open_mode, err);
    }

    lua_pushstring(L, FILE_HANDLE);
    lua_pushlightuserdata(L, file);
    lua_settable(L, 1);

    luaL_getmetatable(L, LUA_FILELIBNAME);
    lua_setmetatable(L, 1);

    return 0;
}

static int file_close(lua_State* L)
{
    return file_gc(L);
}

static int file_read(lua_State* L)
{
    size_t size = 0;
    FILE* file = NULL;
    size_t result = 0;
    luaL_Buffer b;
    char szBuf[512];

    luaL_checktype(L, 1, LUA_TTABLE);
    size = luaL_checkunsigned(L, 2);

    lua_pushstring(L, FILE_HANDLE);
    lua_gettable(L, 1);
    if (LUA_TLIGHTUSERDATA != lua_type(L, -1))
    {
        lua_pop(L, 1);
        luaL_error(L, "must open the file.");
    }

    file = (FILE*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    luaL_buffinitsize(L, &b, size);
    {
        size_t pos = 0;
        while (pos < size)
        {
            memset(szBuf, 0, sizeof(szBuf));
            fread(szBuf, sizeof(szBuf), 1, file);
            result = ferror(file);
            if (result)
                luaL_error(L, "read file fail, errno = %d.", result);

            luaL_addstring(&b, szBuf);
            pos += sizeof(szBuf);
        }
    }

    luaL_pushresult(&b);
    return 1;
}

static int file_write(lua_State* L)
{
    size_t size = 0;
    const char* buff = NULL;
    FILE* file = NULL;
    size_t result = 0;

    luaL_checktype(L, 1, LUA_TTABLE);
    buff = luaL_checkstring(L, 2);
    size = luaL_optinteger(L, 3, 0);
    size = size ? size : strlen(buff);

    lua_pushstring(L, FILE_HANDLE);
    lua_gettable(L, 1);
    if (LUA_TLIGHTUSERDATA != lua_type(L, -1))
    {
        lua_pop(L, 1);
        luaL_error(L, "must open the file.");
    }

    file = (FILE*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    fwrite(buff, size, 1, file);
    result = ferror(file);
    if (result)
        luaL_error(L, "write file fail, errno = %d.", result);

    return 0;
}

static int file_size(lua_State* L)
{
    size_t size = 0;
    FILE* file = NULL;
    size_t result = 0;

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, FILE_HANDLE);
    lua_gettable(L, 1);
    if (LUA_TLIGHTUSERDATA != lua_type(L, -1))
    {
        lua_pop(L, 1);
        luaL_error(L, "must open the file.");
    }

    file = (FILE*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    result = fseek(file, 0, SEEK_END);
    if (result)
        luaL_error(L, "fseek fail, errno = %d.", result);

    size = ftell(file);
    result = ferror(file);
    if (result)
        luaL_error(L, "ftell fail, errno = %d.", result);

    result = fseek(file, 0, SEEK_SET);
    if (result)
        luaL_error(L, "fseek fail, errno = %d.", result);

    lua_pushunsigned(L, size);
    return 1;
}

static int file_gc(lua_State* L)
{
    FILE* file = NULL;
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushstring(L, FILE_HANDLE);
    lua_gettable(L, 1);
    if (LUA_TNIL != lua_type(L, -1))
    {
        fclose((FILE*)lua_touserdata(L, -1));
        lua_pop(L, 1);

        lua_pushstring(L, FILE_HANDLE);
        lua_pushnil(L);
        lua_settable(L, 1);
    }

    return 0;
}

static luaL_Reg file_libs[] = 
{
    {"open", file_open},
    {"close", file_close},
    {"write", file_write},
    {"read", file_read},
    {"size", file_size},
    {NULL, NULL}
};

LUALIB_API int luaopen_file(lua_State* L)
{
    luaL_newlib(L, file_libs);          
     
    luaL_newmetatable(L, LUA_FILELIBNAME); 
    lua_pushstring(L, "__index");       
    lua_pushvalue(L, -3);
    lua_settable(L, -3);

    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, file_gc);
    lua_settable(L, -3);

    lua_pop(L, 1);                      
    return 1;
}