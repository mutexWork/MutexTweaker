#pragma once

#include "Types.h"
#include <vector>
#include <memory>
#include <functional>

namespace MutexTweaker {

class Manager {
public:
    Manager();
    ~Manager();

    const std::vector<TweakInfo>& GetAll() const;
    std::vector<TweakInfo> GetByCategory(Category cat) const;
    std::vector<TweakInfo> GetSafeAll() const;

    TweakResult Apply(const std::string& id);
    TweakResult ApplyByCategory(Category cat);
    TweakResult ApplySafeAll();

    HardwareInfo GetHardware() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace MutexTweaker
