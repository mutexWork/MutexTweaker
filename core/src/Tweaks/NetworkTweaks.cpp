#include "MutexTweaker/Tweaks/NetworkTweaks.h"
#include "MutexTweaker/Utils.h"

namespace MutexTweaker {

static BOOL WINAPI DnsFlushFn() {
    typedef BOOL(WINAPI* F)();
    HMODULE m = LoadLibraryW(L"dnsapi.dll");
    if (!m) return FALSE;
    F f = (F)GetProcAddress(m, "DnsFlushResolverCache");
    BOOL r = f ? f() : FALSE;
    FreeLibrary(m);
    return r;
}

TweakResult NetworkTweaks::DisableNagle() {
    TweakResult r; r.success = true;
#ifdef _WIN32
    const wchar_t* base = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, base, 0, KEY_READ, &hk) != ERROR_SUCCESS) { r.success=false; return r; }
    for (DWORD i=0;;++i) {
        wchar_t name[512]; DWORD nl=511;
        if (RegEnumKeyExW(hk,i,name,&nl,0,0,0,0)==ERROR_NO_MORE_ITEMS) break;
        std::wstring p = std::wstring(base)+L"\\"+name;
        if (!Utils::SetRegDword(HKEY_LOCAL_MACHINE,p,L"TcpAckFrequency",1)) r.success=false;
        if (!Utils::SetRegDword(HKEY_LOCAL_MACHINE,p,L"TCPNoDelay",1)) r.success=false;
    }
    RegCloseKey(hk);
    r.rebootRequired = true;
#endif
    return r;
}

TweakResult NetworkTweaks::FlushDns() {
    TweakResult r;
    r.success = (DnsFlushFn() == TRUE);
    return r;
}

TweakResult NetworkTweaks::RemoveNetworkThrottling() {
    TweakResult r;
    r.success = Utils::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile", L"NetworkThrottlingIndex", 0xFFFFFFFF);
    return r;
}

// Отключает P2P-раздачу обновлений Windows (Delivery Optimization).
TweakResult NetworkTweaks::DisableDeliveryOptimization() {
    TweakResult r;
    bool ok = Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization",
        L"DODownloadMode", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config",
        L"DODownloadMode", 0);
    r.success = ok;
    return r;
}

} // namespace MutexTweaker
