#pragma once

#include "../Types.h"

namespace MutexTweaker {
class InputTweaks {
public:
    static TweakResult TweakMouse();
    static TweakResult TweakKeyboard();
    static TweakResult DisableMouseAcceleration();
};
} // namespace MutexTweaker
