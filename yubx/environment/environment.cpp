#include "environment.hpp"

void register_env_functions(lua_State* l, const luaL_Reg* functions) {
    lua_pushvalue(l, LUA_GLOBALSINDEX);
    luaL_register(l, nullptr, functions);
    lua_pop(l, 1);
}

void register_env_members(lua_State* l, const luaL_Reg* functions, const std::string& global_name) {
    luaL_register(l, global_name.c_str(), functions);
}

void register_to_global(lua_State* l, const luaL_Reg* functions, const std::string& global_name) {
    lua_getglobal(l, global_name.c_str());
    if (lua_istable(l, -1)) {
        lua_setreadonly(l, -1, false);
        luaL_register(l, nullptr, functions);
        lua_setreadonly(l, -1, true);
    }
    lua_pop(l, 1);
}

int identifyexecutor(lua_State* L) {
    luaL_trimstack(L, 0);
    lua_pushstring(L, "yubx");
    lua_pushstring(L, "2.0.0");
    return 2;
};

int getexecutorname(lua_State* L) {
    luaL_trimstack(L, 0);
    lua_pushstring(L, "yubx");
    return 1;
};

void environment::initialize(lua_State* l)
{

    static const luaL_Reg yubx_misc[] = {
      { "identifyexecutor", identifyexecutor },
      { "getexecutorname",  getexecutorname  },
      { nullptr, nullptr }
    };
    register_env_functions(l, yubx_misc);

    lua_newtable(l);
    lua_setglobal(l, "_G");

    lua_newtable(l);
    lua_setglobal(l, "shared");
}