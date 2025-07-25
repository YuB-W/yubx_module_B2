#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <thread>
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>

#include "Environment/Environment.hpp"
#include "Execution/Execution.hpp"
#include "Communication.hpp"
#include "../TP_hanlder/TP_hanlder.hpp"

std::wstring GetPipeName() {
    DWORD pid = GetCurrentProcessId();
    return L"YuBX_" + std::to_wstring(pid);
}

namespace {
    constexpr DWORD PIPE_TIMEOUT_MS = 5000;
    constexpr DWORD MAX_SCRIPT_SIZE = 8 * 1024 * 1024; 

    bool ReadExact(HANDLE hPipe, void* buffer, DWORD size) {
        DWORD totalRead = 0;
        while (totalRead < size) {
            DWORD chunk = 0;
            if (!ReadFile(hPipe, static_cast<char*>(buffer) + totalRead, size - totalRead, &chunk, nullptr) || chunk == 0)
                return false;
            totalRead += chunk;
        }
        return true;
    }

    void NamedPipeServer(const std::wstring& pipeName) {
        const std::wstring fullPipePath = L"\\\\.\\pipe\\" + pipeName;

        while (true) {
            HANDLE hPipe = CreateNamedPipeW(
                fullPipePath.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                1, MAX_SCRIPT_SIZE, MAX_SCRIPT_SIZE,
                PIPE_TIMEOUT_MS,
                nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                Sleep(100);
                continue;
            }

            BOOL connected = ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
            if (!connected) {
                CloseHandle(hPipe);
                continue;
            }

            uint32_t scriptSize = 0;
            if (!ReadExact(hPipe, &scriptSize, sizeof(scriptSize)) || scriptSize == 0 || scriptSize > MAX_SCRIPT_SIZE) {
                CloseHandle(hPipe);
                continue;
            }

            std::vector<char> buffer(scriptSize);
            if (!ReadExact(hPipe, buffer.data(), scriptSize)) {
                CloseHandle(hPipe);
                continue;
            }

            std::string receivedScript(buffer.data(), scriptSize);
            teleport_handler tp;
            if (!tp.is_home_page()) {
                execution::execute_script(globals::yubx_state, receivedScript);
            }

            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }
}

void CCommunication::Initialize() {
    std::wstring pipeName = GetPipeName();  
    std::thread([=] { NamedPipeServer(pipeName); }).detach();
}
