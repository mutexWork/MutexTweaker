#include "MutexTweaker/Tweaks/InputTweaks.h"
#include "MutexTweaker/Utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace MutexTweaker {

TweakResult InputTweaks::TweakMouse() {
    TweakResult r;
    r.success = Utils::SetRegString(HKEY_CURRENT_USER, L"Control Panel\\Mouse",
                                    L"MouseHoverTime", L"100");
#ifdef _WIN32
    // Применяем сразу, не дожидаясь входа в систему.
    SystemParametersInfoW(SPI_SETMOUSEHOVERTIME, 100, nullptr,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
#endif
    return r;
}

TweakResult InputTweaks::TweakKeyboard() {
    TweakResult r;
    r.success = Utils::SetRegString(HKEY_CURRENT_USER, L"Control Panel\\Keyboard",
                                    L"KeyboardDelay", L"0");
#ifdef _WIN32
    SystemParametersInfoW(SPI_SETKEYBOARDDELAY, 0, nullptr,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
#endif
    return r;
}

// Отключает ускорение мыши (pointer acceleration).
TweakResult InputTweaks::DisableMouseAcceleration() {
    TweakResult r;
    bool ok = Utils::SetRegString(HKEY_CURRENT_USER, L"Control Panel\\Mouse",
                                  L"MouseSpeed", L"0");
    ok &= Utils::SetRegString(HKEY_CURRENT_USER, L"Control Panel\\Mouse",
                              L"MouseThreshold1", L"0");
    ok &= Utils::SetRegString(HKEY_CURRENT_USER, L"Control Panel\\Mouse",
                              L"MouseThreshold2", L"0");
#ifdef _WIN32
    int params[3] = { 0, 0, 0 };
    if (!SystemParametersInfoW(SPI_SETMOUSE, 0, params,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE))
    {
        ok = false;
    }
#endif
    r.success = ok;
    return r;
}

} // namespace MutexTweaker
