#pragma once
#include <cstdint>
#include <string>
#include <Windows.h>
#include <iostream>

#include "shuffles-encryptions.hpp"

struct lua_State;
#define xreb(x) (x + (uintptr_t)(GetModuleHandle(nullptr)))

namespace update
{
    namespace roblox
    {
        const uintptr_t print = xreb(0x156CBB0);
        const uintptr_t OpcodeLookupTable = xreb(0x510c570);

        const uintptr_t task_scheduler = xreb(0x63A0968);
        const uintptr_t fake_datamodel = xreb(0x67D0C18);
        const uintptr_t luad_throw = xreb(0x26665E0);
        const uintptr_t task_defer = xreb(0xFE4A60);
    }

    namespace lua
    {
        const uintptr_t luao_nilobject = xreb(0x46F9AC8);
        const uintptr_t luau_execute = xreb(0x2698E80);
        const uintptr_t luah_dummynode = xreb(0x46F94E8);
    }

    namespace lua_state
    {
        const uintptr_t GetGlobalState = xreb(0xDC1FE0);
    }

    namespace offsets
    {
        namespace datamodel
        {
            const uintptr_t fake_datamodel_to_datamodel = 0x1B8;
            const uintptr_t script_context = 0x3B0;
        }

        namespace luastate
        {
            const uintptr_t global_state = 0x140;
            const uintptr_t decrypt_state = 0x88;
        }

        namespace instance
        {
            const uintptr_t name = 0x78;
            const uintptr_t children = 0x80;
        }
    }
}

namespace roblox
{
    using print_func_t = int(__fastcall*)(int, const char*, ...);
    inline print_func_t r_print = reinterpret_cast<print_func_t>(update::roblox::print);

    using luad_throw_t = void(__fastcall*)(lua_State*, int);
    inline luad_throw_t luad_throw = reinterpret_cast<luad_throw_t>(update::roblox::luad_throw);

    using task_defer_t = uintptr_t(__fastcall*)(int64_t);
    inline task_defer_t task_defer = reinterpret_cast<task_defer_t>(update::roblox::task_defer);

    inline auto GetState = (uintptr_t(__fastcall*)(int64_t, uint64_t*, uint64_t*))update::lua_state::GetGlobalState;
}