#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace MutexTweaker {

enum class Category {
    Network = 0,
    Power = 1,
    Services = 2,
    Privacy = 3,
    Explorer = 4,
    Input = 5,
    Storage = 6,
    Latency = 7,
    SystemInfo = 8
};

struct TweakResult {
    bool success = false;
    bool rebootRequired = false;
    std::wstring message;
};

struct TweakInfo {
    std::string id;
    std::wstring name;
    std::wstring description;
    Category category;
    bool isSafe = true;
    bool canRevert = false;
};

struct HardwareInfo {
    std::wstring cpuName;
    std::wstring gpuName;
    std::wstring motherboardName;
    double ramGb = 0.0;
    double vramGb = 0.0;
    std::wstring ramType;
    int ramSpeed = 0;
    std::wstring systemDiskType;
    std::vector<std::wstring> networkAdapters;
};

} // namespace MutexTweaker
