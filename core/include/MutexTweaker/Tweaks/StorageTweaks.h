#pragma once

#include "../Types.h"

namespace MutexTweaker {
class StorageTweaks {
public:
    static TweakResult CleanUserTemp();
    static TweakResult DisableHibernation();
    static TweakResult CleanSystemTemp();
};
} // namespace MutexTweaker
