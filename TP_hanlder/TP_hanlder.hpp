#pragma once
#include <cstdint>

struct lua_State;

class teleport_handler {
private:
    uintptr_t last_datamodel = 0;
    uintptr_t last_script_context = 0;
    uintptr_t last_place_id = 0;
    uintptr_t last_game_id = 0;
    lua_State* last_yubx_state = nullptr;
    bool has_initialized = false;

public:
    void init();
    bool is_home_page();
    bool detect_teleport();
    bool entered_game();
    bool should_initialize();
    void mark_initialized();
    void start_teleport_watch();
    bool lua_state_changed();  // 🔹 NEW METHOD
};
