#include "MutexTweaker/Tweaks/LatencyTweaks.h"
#include "MutexTweaker/Utils.h"

namespace MutexTweaker {

TweakResult LatencyTweaks::ReduceInterruptLatencySafe() {
    TweakResult r;
    r.success = Utils::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile", L"NetworkThrottlingIndex", 0xFFFFFFFF);
    return r;
}

// Отдаёт ресурсы мультимедиа-планировщика играм и убирает резерв под службы.
TweakResult LatencyTweaks::OptimizeSystemResponsiveness() {
    TweakResult r;
    bool ok = true;
    const wchar_t* profile = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";

    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, profile, L"SystemResponsiveness", 0);

    const wchar_t* games = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Games";
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, games, L"GPU Priority", 8);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, games, L"Priority", 6);
    ok &= Utils::SetRegString(HKEY_LOCAL_MACHINE, games, L"Scheduling Category", L"High");

    const wchar_t* tasks = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, tasks, L"GPU Priority", 8);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, tasks, L"Priority", 6);
    ok &= Utils::SetRegString(HKEY_LOCAL_MACHINE, tasks, L"Scheduling Category", L"High");

    r.success = ok;
    return r;
}

} // namespace MutexTweaker
