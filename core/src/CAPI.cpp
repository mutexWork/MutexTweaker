#include "MutexTweaker/CAPI.h"
#include "MutexTweaker/Manager.h"
#include "MutexTweaker/SystemRestore.h"
#include <string>

using namespace MutexTweaker;

static Manager* g_mgr = nullptr;
static HardwareInfo g_hw;

extern "C" {

WO_API void WO_Init() {
    if (!g_mgr) {
        g_mgr = new Manager();
        g_hw = g_mgr->GetHardware();
    }
}

WO_API int WO_GetTweakCount() {
    if (!g_mgr) WO_Init();
    return (int)g_mgr->GetAll().size();
}

WO_API const char* WO_GetTweakId(int index) {
    if (!g_mgr) WO_Init();
    static std::string s;
    auto& list = g_mgr->GetAll();
    if (index < 0 || index >= (int)list.size()) return "";
    s = list[index].id;
    return s.c_str();
}

WO_API const wchar_t* WO_GetTweakName(int index) {
    if (!g_mgr) WO_Init();
    auto& list = g_mgr->GetAll();
    if (index < 0 || index >= (int)list.size()) return L"";
    return list[index].name.c_str();
}

WO_API const wchar_t* WO_GetTweakDesc(int index) {
    if (!g_mgr) WO_Init();
    auto& list = g_mgr->GetAll();
    if (index < 0 || index >= (int)list.size()) return L"";
    return list[index].description.c_str();
}

WO_API int WO_GetTweakCategory(int index) {
    if (!g_mgr) WO_Init();
    auto& list = g_mgr->GetAll();
    if (index < 0 || index >= (int)list.size()) return 0;
    return (int)list[index].category;
}

WO_API int WO_ApplyTweak(const char* id) {
    if (!g_mgr) WO_Init();
    if (!id) return 0;
    auto r = g_mgr->Apply(id);
    return r.success ? 1 : 0;
}

WO_API int WO_ApplyAllSafe() {
    if (!g_mgr) WO_Init();
    auto r = g_mgr->ApplySafeAll();
    return r.success ? 1 : 0;
}

WO_API int WO_ApplyByCategory(int cat) {
    if (!g_mgr) WO_Init();
    auto r = g_mgr->ApplyByCategory((Category)cat);
    return r.success ? 1 : 0;
}

WO_API int WO_CreateRestorePoint(const wchar_t* description,
                                 wchar_t* errorBuf, int errorBufLen) {
    std::wstring err;
    bool ok = SystemRestore::Create(description ? description : L"MutexTweaker", &err);

    if (!ok && errorBuf && errorBufLen > 0) {
        wcsncpy_s(errorBuf, (size_t)errorBufLen, err.c_str(), _TRUNCATE);
    } else if (ok && errorBuf && errorBufLen > 0) {
        errorBuf[0] = L'\0';
    }
    return ok ? 1 : 0;
}

WO_API const wchar_t* WO_GetCpuName() { if (!g_mgr) WO_Init(); return g_hw.cpuName.c_str(); }
WO_API const wchar_t* WO_GetGpuName() { if (!g_mgr) WO_Init(); return g_hw.gpuName.c_str(); }
WO_API double WO_GetRamGb() { if (!g_mgr) WO_Init(); return g_hw.ramGb; }
WO_API double WO_GetVramGb() { if (!g_mgr) WO_Init(); return g_hw.vramGb; }
WO_API const wchar_t* WO_GetRamType() { if (!g_mgr) WO_Init(); return g_hw.ramType.c_str(); }
WO_API int WO_GetRamSpeed() { if (!g_mgr) WO_Init(); return g_hw.ramSpeed; }
WO_API const wchar_t* WO_GetDiskType() { if (!g_mgr) WO_Init(); return g_hw.systemDiskType.c_str(); }

}
