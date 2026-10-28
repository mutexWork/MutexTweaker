#pragma once

#include "../Types.h"

namespace MutexTweaker {
class PrivacyTweaks {
public:
    static TweakResult LimitTelemetrySafe();
    static TweakResult DisableGameDVR();
    static TweakResult LimitAdsAndSuggestions();
    static TweakResult DisableCortana();
    static TweakResult DisableActivityHistory();
    static TweakResult DisableFeedback();
};
} // namespace MutexTweaker
