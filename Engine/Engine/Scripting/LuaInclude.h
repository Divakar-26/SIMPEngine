#pragma once

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#if LUA_VERSION_NUM < 504
#define lua_newuserdatauv(L, size, nuvalue) lua_newuserdata(L, size)
#endif
