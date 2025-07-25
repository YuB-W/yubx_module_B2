#include <roblox/task_scheduler/task_scheduler.hpp>
#include <execution/execution.hpp>
#include <thread>
#include <environment/environment.hpp>
#include "TP_hanlder.hpp"

constexpr uintptr_t OFFSET_PLACE_ID = 0x1A0;
constexpr uintptr_t OFFSET_GAME_ID  = 0x198;

void teleport_handler::init() {
    last_datamodel = task_scheduler::get_datamodel();
    last_script_context = task_scheduler::get_script_context();
    last_place_id = *(uintptr_t*)(last_datamodel + OFFSET_PLACE_ID);
    last_game_id = *(uintptr_t*)(last_datamodel + OFFSET_GAME_ID);
    last_yubx_state = globals::yubx_state;
}

bool teleport_handler::entered_game() {
    uintptr_t dm = task_scheduler::get_datamodel();
    if (!dm) return false;

    uintptr_t place_id = *(uintptr_t*)(dm + OFFSET_PLACE_ID);
    uintptr_t game_id = *(uintptr_t*)(dm + OFFSET_GAME_ID);

    if (place_id != 0 && game_id != 0 &&
        (place_id != last_place_id || game_id != last_game_id)) {
        last_place_id = place_id;
        last_game_id = game_id;
        return true;
    }

    return false;
}

bool teleport_handler::should_initialize() {
    return !has_initialized && !is_home_page() &&
        (detect_teleport() || entered_game() || lua_state_changed());
}

void teleport_handler::mark_initialized() {
    has_initialized = true;
}

bool teleport_handler::is_home_page() {
    uintptr_t dm = task_scheduler::get_datamodel();
    if (!dm) return true;

    uintptr_t place_id = *(uintptr_t*)(dm + OFFSET_PLACE_ID);
    uintptr_t game_id = *(uintptr_t*)(dm + OFFSET_GAME_ID);

    return (place_id == 0 || game_id == 0);
}

bool teleport_handler::detect_teleport() {
    uintptr_t dm = task_scheduler::get_datamodel();
    uintptr_t sc = task_scheduler::get_script_context();

    bool changed = (dm && dm != last_datamodel) || (sc && sc != last_script_context);
    if (changed) {
        last_datamodel = dm;
        last_script_context = sc;
    }
    return changed;
}

bool teleport_handler::lua_state_changed() {
    lua_State* current_lua = (lua_State*)(task_scheduler::get_lua_state());

    if (current_lua != last_yubx_state) {
        last_yubx_state = current_lua;
        return true;
    }

    return false;
}

void reinit_lua() {
    globals::global_state = (lua_State*)(task_scheduler::get_lua_state());
    globals::yubx_state = lua_newthread(globals::global_state);
    task_scheduler::set_thread_capabilities(globals::yubx_state, 8, max_caps);
    luaL_sandboxthread(globals::yubx_state);
    environment::initialize(globals::yubx_state);
}

void teleport_handler::start_teleport_watch() {
    teleport_handler tp;
    tp.init();

    while (true) {
        if (tp.is_home_page()) {
            tp = teleport_handler();  // reset
            tp.init();
        }

        if (tp.should_initialize()) {
            tp.mark_initialized();
            reinit_lua();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
