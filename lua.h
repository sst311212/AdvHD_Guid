typedef struct lua_State lua_State;
typedef lua_State* (*_luaL_newstate)();
_luaL_newstate luaL_newstate = NULL;

typedef void (*_luaL_openlibs)(lua_State* L);
_luaL_openlibs luaL_openlibs = NULL;

typedef int (*_luaL_loadbuffer)(lua_State* L, const char* buff, size_t sz, const char* name);
_luaL_loadbuffer luaL_loadbuffer = NULL;

typedef int (*_luaL_loadbufferx)(lua_State* L, const char* buff, size_t sz, const char* name, const char* mode);
_luaL_loadbufferx luaL_loadbufferx = NULL;

typedef int (*_lua_pcall)(lua_State* L, int nargs, int nresults, int errfunc);
_lua_pcall lua_pcall = NULL;

#define LUA_KCONTEXT ptrdiff_t
typedef LUA_KCONTEXT lua_KContext;
typedef int (*lua_KFunction) (lua_State* L, int status, lua_KContext ctx);
typedef int (*_lua_pcallk)(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k);
_lua_pcallk lua_pcallk = NULL;

typedef void (*_lua_pushnil)(lua_State* L);
_lua_pushnil lua_pushnil = NULL;

typedef int (*_lua_getfield)(lua_State* L, int idx, const char* k);
_lua_getfield lua_getfield = NULL;

typedef int (*_lua_getglobal)(lua_State* L, const char* name);
_lua_getglobal lua_getglobal = NULL;

typedef const char* (*_lua_tolstring)(lua_State* L, int idx, size_t* len);
_lua_tolstring lua_tolstring = NULL;

#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)