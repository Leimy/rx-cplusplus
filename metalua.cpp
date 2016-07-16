#include "metalua.hpp"
#include "lua.hpp"
#include <stdlib.h>

extern "C" {
int metalua_getmetadata (lua_State *L) {
  char * res = getMetaData();
  lua_pushstring(L, res);
  free(res);
  return 1;
}

static const luaL_Reg metalib [] = {
  {"getMetaData", metalua_getmetadata},
  {NULL, NULL}
};

LUAMOD_API int luaopen_libmetalua (lua_State *L) {
  luaL_newlib(L, metalib);
  return 1;
}

}
