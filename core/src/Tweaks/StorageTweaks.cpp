#include "MutexTweaker/Tweaks/StorageTweaks.h"
#include <filesystem>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace MutexTweaker {

static void CleanDirectory(const fs::path& dir)
{
    std::error_code ec;
    if (!fs::exists(dir, ec)) return;

    for (auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
    {
        std::error_code iec;
        fs::remove_all(entry.path(), iec);
    }
}

TweakResult StorageTweaks::CleanUserTemp() {
    TweakResult r;
    r.success = true;
#ifdef _WIN32
    wchar_t buf[4096];
    DWORD n = GetTempPathW(4096, buf);
    if (n == 0) {
        r.success = false;
        return r;
    }
    CleanDirectory(fs::path(buf));
#else
    r.success = false;
#endif
    return r;
}

// Отключает гибернацию и удаляет hiberfil.sys, освобождая место на диске.
TweakResult StorageTweaks::DisableHibernation() {
    TweakResult r;
    r.success = false;
#ifdef _WIN32
    wchar_t cmd[] = L"powercfg.exe /h off";
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, 20000);
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        r.success = (code == 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif
    return r;
}

// Очищает системную папку C:\Windows\Temp.
TweakResult StorageTweaks::CleanSystemTemp() {
    TweakResult r;
    r.success = false;
#ifdef _WIN32
    wchar_t winDir[MAX_PATH] = {};
    UINT n = GetWindowsDirectoryW(winDir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return r;

    fs::path dir = fs::path(winDir) / L"Temp";
    std::error_code ec;
    if (!fs::exists(dir, ec)) return r;

    CleanDirectory(dir);
    r.success = true;
#endif
    return r;
}

} // namespace MutexTweaker
