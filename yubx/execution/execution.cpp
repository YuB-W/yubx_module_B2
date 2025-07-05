#include "execution.hpp"
#include <chrono>
#include <string>
#include <vector>

std::string CompileScript(const std::string Source) {
    auto BytecodeEncoder = bytecode_encoder_t();

    Luau::CompileOptions options;
    options.debugLevel = 1;
    options.optimizationLevel = 1;
    const char* mutableGlobals[] = {
        "Game", "Workspace", "game", "plugin", "script", "shared", "workspace",
        "_G", "_ENV", nullptr
    };
    options.mutableGlobals = mutableGlobals;
    options.vectorLib = "Vector3";
    options.vectorCtor = "new";
    options.vectorType = "Vector3";

    return Luau::compile(Source, options, {}, &BytecodeEncoder);
}

void execution::execute_script(lua_State* l, const std::string& script)
{
    auto Bytecode = CompileScript(script);

    lua_settop(l, 0);
    auto execution_thread = lua_newthread(l);

    luaL_sandboxthread(execution_thread);

    if (luau_load(execution_thread, "", Bytecode.c_str(), Bytecode.length(), 0) != LUA_OK) {
        std::string errMsg = luaL_checklstring(execution_thread, -1, nullptr);
        roblox::r_print(2, errMsg.c_str());
    }


    task_scheduler::set_proto_capabilities(((Closure*)lua_topointer(execution_thread, -1))->l.p, &max_caps);
    roblox::task_defer((int64_t)execution_thread);

    lua_settop(execution_thread, 0);
    lua_pop(l, 1);
}

