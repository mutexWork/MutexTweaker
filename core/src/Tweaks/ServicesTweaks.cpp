#include "MutexTweaker/Tweaks/ServicesTweaks.h"
#include "MutexTweaker/Utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsvc.h>
#pragma comment(lib, "advapi32.lib")
#endif

namespace MutexTweaker {

#ifdef _WIN32
// Отключает автозапуск службы через реестр и SCM, затем пытается её остановить.
static TweakResult DisableService(const wchar_t* serviceName)
{
    TweakResult r;
    r.success = false;

    std::wstring key = std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\") + serviceName;
    bool regOk = Utils::SetRegDword(HKEY_LOCAL_MACHINE, key, L"Start", 4);

    bool scOk = false;
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scm)
    {
        SC_HANDLE svc = OpenServiceW(scm, serviceName,
            SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (svc)
        {
            if (ChangeServiceConfigW(svc, SERVICE_NO_CHANGE, SERVICE_DISABLED,
                                     SERVICE_NO_CHANGE, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, nullptr))
            {
                scOk = true;
            }

            SERVICE_STATUS st = {};
            ControlService(svc, SERVICE_CONTROL_STOP, &st);
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(scm);
    }

    r.success = regOk || scOk;
    r.rebootRequired = false;
    return r;
}
#endif

TweakResult ServicesTweaks::DisableSysMain() {
#ifdef _WIN32
    return DisableService(L"SysMain");
#else
    return TweakResult{};
#endif
}

TweakResult ServicesTweaks::DisableDiagTrack() {
#ifdef _WIN32
    return DisableService(L"DiagTrack");
#else
    return TweakResult{};
#endif
}

TweakResult ServicesTweaks::DisableWerSvc() {
    TweakResult r;
    bool ok = Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting", L"Disabled", 1);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting", L"Disabled", 1);
#ifdef _WIN32
    TweakResult svc = DisableService(L"WerSvc");
    r.success = ok || svc.success;
#else
    r.success = ok;
#endif
    return r;
}

} // namespace MutexTweaker
