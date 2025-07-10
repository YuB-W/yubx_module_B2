#include "task_scheduler.hpp"

void task_scheduler::set_proto_capabilities(Proto* proto, uintptr_t* c)
{
    if (!proto) return;

    proto->userdata = c;
    for (int i = 0; i < proto->sizep; ++i)
        set_proto_capabilities(proto->p[i], c);
}

void task_scheduler::set_thread_capabilities(lua_State* l, int lvl, uintptr_t c)
{
    if (!l || !l->userdata) return;

    auto extra_space = (uintptr_t)(l->userdata);
    *(uintptr_t*)(extra_space + 0x48) = c;
    *(int*)(extra_space + 0x30) = lvl;
}

uintptr_t task_scheduler::get_datamodel()
{
    uintptr_t fake_datamodel = *(uintptr_t*)(update::roblox::fake_datamodel);
    return *(uintptr_t*)(fake_datamodel + update::offsets::datamodel::fake_datamodel_to_datamodel);
}

uintptr_t task_scheduler::get_script_context() {
    uintptr_t datamodel = task_scheduler::get_datamodel();
    uintptr_t childcontainer = *(uintptr_t*)(datamodel + update::offsets::instance::children);
    uintptr_t children = *(uintptr_t*)(childcontainer);
    return *(uintptr_t*)(children + update::offsets::datamodel::script_context);
}

inline lua_State* DecodeLuaState(uintptr_t globalstate) {
    size_t offset = 48;
    uintptr_t GlobalState = globalstate + offset;
    const uint32_t* ptr = reinterpret_cast<const uint32_t*>(GlobalState + 0x88);
    const uint32_t DecryptState = static_cast<uint32_t>(GlobalState + 0x88);
    return reinterpret_cast<lua_State*>((static_cast<uint64_t>(ptr[1] ^ DecryptState) << 32) | (ptr[0] ^ DecryptState));
}

uintptr_t task_scheduler::get_lua_state() {
    uintptr_t scriptContext = get_script_context();
    return (uintptr_t)DecodeLuaState(scriptContext + update::offsets::luastate::global_state);
}
