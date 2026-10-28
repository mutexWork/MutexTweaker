#pragma once

#include "../Types.h"

namespace MutexTweaker {
class LatencyTweaks {
public:
    static TweakResult ReduceInterruptLatencySafe();
    static TweakResult OptimizeSystemResponsiveness();
};
} // namespace MutexTweaker
