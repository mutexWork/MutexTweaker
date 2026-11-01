#include "MutexTweaker/Tweaks/PrivacyTweaks.h"
#include "MutexTweaker/Utils.h"

namespace MutexTweaker {

TweakResult PrivacyTweaks::LimitTelemetrySafe() {
    TweakResult r;
    bool ok = true;
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowTelemetry", 0);
    r.success = ok;
    return r;
}

TweakResult PrivacyTweaks::DisableGameDVR() {
    TweakResult r;
    bool ok = true;
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR", L"AppCaptureEnabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, L"System\\GameConfigStore", L"GameDVR_Enabled", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR", L"AllowGameDVR", 0);
    r.success = ok;
    return r;
}

// Отключает рекламный идентификатор и подсказки/рекомендации в меню «Пуск».
TweakResult PrivacyTweaks::LimitAdsAndSuggestions() {
    TweakResult r;
    bool ok = true;
    const wchar_t* cdm = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager";

    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo", L"Enabled", 0);

    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SilentInstalledAppsEnabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SystemPaneSuggestionsEnabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SoftLandingEnabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SubscribedContent-338388Enabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SubscribedContent-338389Enabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SubscribedContent-353694Enabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"SubscribedContent-353696Enabled", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER, cdm, L"RotatingLockScreenOverlayEnabled", 0);

    r.success = ok;
    return r;
}

// Отключает Cortana и веб-результаты в поиске.
TweakResult PrivacyTweaks::DisableCortana() {
    TweakResult r;
    bool ok = true;
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortana", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowSearchToUseLocation", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"CortanaConsent", 0);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"BingSearchEnabled", 0);
    r.success = ok;
    return r;
}

// Отключает журнал действий и отправку истории активности.
TweakResult PrivacyTweaks::DisableActivityHistory() {
    TweakResult r;
    bool ok = true;
    const wchar_t* sys = L"SOFTWARE\\Policies\\Microsoft\\Windows\\System";
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, sys, L"EnableActivityFeed", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, sys, L"PublishUserActivities", 0);
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE, sys, L"UploadUserActivities", 0);
    r.success = ok;
    return r;
}

// Отключает запросы на отзывы и опросы Windows.
TweakResult PrivacyTweaks::DisableFeedback() {
    TweakResult r;
    bool ok = true;
    ok &= Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection",
        L"DoNotShowFeedbackNotifications", 1);
    ok &= Utils::SetRegDword(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Siuf\\Rules", L"NumberOfSIUFInPeriod", 0);
    r.success = ok;
    return r;
}

} // namespace MutexTweaker
