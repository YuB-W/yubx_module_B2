#include "Windows.h"
#include "thread"
#include <environment/environment.hpp>
#include <roblox/task_scheduler/task_scheduler.hpp>
#include <execution/execution.hpp>

void main()
{
    globals::global_state = (lua_State*)(task_scheduler::get_lua_state());
    globals::yubx_state = lua_newthread(globals::global_state);

    task_scheduler::set_thread_capabilities(globals::yubx_state, 8, max_caps);
    luaL_sandboxthread(globals::yubx_state);
    environment::initialize(globals::yubx_state);

    execution::execute_script(globals::yubx_state, R"--(
        printidentity()
        print(identifyexecutor())
    )--");
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID res)
{
    if (reason == DLL_PROCESS_ATTACH)
        std::thread(main).detach();
    return TRUE;
}