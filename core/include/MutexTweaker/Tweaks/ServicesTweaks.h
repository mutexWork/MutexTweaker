#pragma once

#include "../Types.h"

namespace MutexTweaker {
class ServicesTweaks {
public:
    static TweakResult DisableSysMain();
    static TweakResult DisableDiagTrack();
    static TweakResult DisableWerSvc();
};
} // namespace MutexTweaker
