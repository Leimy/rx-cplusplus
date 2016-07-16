#ifndef __METALUA_H_
#define __METALUA_H_
#include "lua.hpp"

extern "C" {
extern "C" char *getMetaData();

LUALIB_API int luaopen_libmetalua (lua_State *L);
}
#endif //__METALUA_H_
