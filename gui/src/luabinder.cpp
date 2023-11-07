#include "luabinder.h"

int lookup(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	const char* key = luaL_checkstring(L, 2);

	// Do not have "inherits"?
	luaL_getmetafield(L, 1, "inherits");
	if (lua_type(L, -1) == LUA_TNIL) //lua_isnoneornil(L, -1)
		return 1;

	// Iterate over them
	lua_pushnil(L);
	while (lua_next(L, -2))
	{
		luaL_getmetatable(L, lua_tostring(L, -1));
		lua_getfield(L, -1, "__index");
		lua_getfield(L, -1, key);

		// Found?
		if (lua_type(L, -1) != LUA_TNIL)
		{
			lua_insert(L, 3);
			lua_settop(L, 3);

			return 1;
		}

		lua_pop(L, 4);
	}

	lua_pop(L, 1);
	lua_pushnil(L);

	return 1;
}
