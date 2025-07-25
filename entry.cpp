#include <Windows.h>
#include <thread>
#include <chrono>
#include "TP_hanlder/TP_hanlder.hpp"
#include "Communication/Communication.hpp"

void entry_point() {
    std::thread([]() {
        teleport_handler tp;
        tp.init();
        tp.start_teleport_watch();
        }).detach();

    Communication->Initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule); 
        std::thread(entry_point).detach();
    }
    return TRUE;
}
