#pragma once

#include "../Types.h"

namespace MutexTweaker {
class NetworkTweaks {
public:
    static TweakResult DisableNagle();
    static TweakResult FlushDns();
    static TweakResult RemoveNetworkThrottling();
    static TweakResult DisableDeliveryOptimization();
};
} // namespace MutexTweaker
