#include "MutexTweaker/Tweaks/ExplorerTweaks.h"
#include "MutexTweaker/Utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#endif

namespace MutexTweaker {

#ifdef _WIN32
// Сообщает оболочке, что настройки Проводника изменились,
// чтобы они применились без перезапуска системы.
static void RefreshShell()
{
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    DWORD_PTR result = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"ShellState"),
                        SMTO_ABORTIFHUNG, 1000, &result);
}
#endif

TweakResult ExplorerTweaks::ShowFileExtensions() {
    TweakResult r;
    r.success = Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
        L"HideFileExt", 0);
#ifdef _WIN32
    if (r.success) RefreshShell();
#endif
    return r;
}

TweakResult ExplorerTweaks::ShowHiddenFiles() {
    TweakResult r;
    bool ok = Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
        L"Hidden", 1);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
        L"ShowSuperHidden", 1);
    r.success = ok;
#ifdef _WIN32
    if (r.success) RefreshShell();
#endif
    return r;
}

TweakResult ExplorerTweaks::OpenThisPC() {
    TweakResult r;
    r.success = Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
        L"LaunchTo", 1);
#ifdef _WIN32
    if (r.success) RefreshShell();
#endif
    return r;
}

// Не восстанавливать приложения после перезагрузки/входа в систему.
TweakResult ExplorerTweaks::DisableRestartApps() {
    TweakResult r;
    r.success = Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"RestartApps", 0);
    return r;
}

} // namespace MutexTweaker
