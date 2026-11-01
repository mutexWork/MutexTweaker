#include "MutexTweaker/SystemInfo.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <winioctl.h>
#include <vector>
#include <algorithm>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

namespace MutexTweaker {

#ifdef _WIN32

// Выполняет WQL-запрос и возвращает значение первого строкового свойства.
static std::wstring WMIString(const std::wstring& q, const std::wstring& p) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool init = (hr == S_OK || hr == S_FALSE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return L"";

    IWbemLocator* loc = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&loc))) {
        if (init) CoUninitialize();
        return L"";
    }

    IWbemServices* svc = nullptr;
    if (FAILED(loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), 0, 0, 0, 0, 0, 0, &svc))) {
        loc->Release(); if (init) CoUninitialize(); return L"";
    }
    if (FAILED(CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return L"";
    }

    IEnumWbemClassObject* en = nullptr;
    if (FAILED(svc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(q.c_str()),
                              WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &en))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return L"";
    }

    std::wstring res;
    IWbemClassObject* obj = nullptr;
    ULONG ret = 0;
    if (en->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && ret) {
        VARIANT v; VariantInit(&v);
        if (SUCCEEDED(obj->Get(p.c_str(), 0, &v, 0, 0)) && v.vt == VT_BSTR && v.bstrVal) res = v.bstrVal;
        VariantClear(&v); obj->Release();
    }
    en->Release(); svc->Release(); loc->Release(); if (init) CoUninitialize();
    return res;
}

// Возвращает все строки свойства из запроса.
static std::vector<std::wstring> WMIStrings(const std::wstring& q, const std::wstring& p) {
    std::vector<std::wstring> out;
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool init = (hr == S_OK || hr == S_FALSE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return out;

    IWbemLocator* loc = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&loc))) {
        if (init) CoUninitialize();
        return out;
    }

    IWbemServices* svc = nullptr;
    if (FAILED(loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), 0, 0, 0, 0, 0, 0, &svc))) {
        loc->Release(); if (init) CoUninitialize(); return out;
    }
    if (FAILED(CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return out;
    }

    IEnumWbemClassObject* en = nullptr;
    if (FAILED(svc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(q.c_str()),
                              WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &en))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return out;
    }

    IWbemClassObject* obj = nullptr;
    ULONG ret = 0;
    while (en->Next(2000, 1, &obj, &ret) == S_OK && ret) {
        VARIANT v; VariantInit(&v);
        if (SUCCEEDED(obj->Get(p.c_str(), 0, &v, 0, 0))) {
            if (v.vt == VT_BSTR && v.bstrVal) out.push_back(v.bstrVal);
            else if (v.vt == VT_I4) out.push_back(std::to_wstring(v.lVal));
            else if (v.vt == VT_UI4) out.push_back(std::to_wstring(v.ulVal));
        }
        VariantClear(&v);
        obj->Release();
        obj = nullptr;
    }
    en->Release(); svc->Release(); loc->Release(); if (init) CoUninitialize();
    return out;
}

// Возвращает первое целочисленное свойство.
static long WMIInt(const std::wstring& q, const std::wstring& p) {
    long res = 0;
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool init = (hr == S_OK || hr == S_FALSE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return 0;

    IWbemLocator* loc = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&loc))) {
        if (init) CoUninitialize();
        return 0;
    }
    IWbemServices* svc = nullptr;
    if (FAILED(loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), 0, 0, 0, 0, 0, 0, &svc))) {
        loc->Release(); if (init) CoUninitialize(); return 0;
    }
    if (FAILED(CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return 0;
    }
    IEnumWbemClassObject* en = nullptr;
    if (FAILED(svc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(q.c_str()),
                              WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &en))) {
        svc->Release(); loc->Release(); if (init) CoUninitialize(); return 0;
    }
    IWbemClassObject* obj = nullptr;
    ULONG ret = 0;
    if (en->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && ret) {
        VARIANT v; VariantInit(&v);
        if (SUCCEEDED(obj->Get(p.c_str(), 0, &v, 0, 0))) {
            if (v.vt == VT_I4) res = v.lVal;
            else if (v.vt == VT_UI4) res = (long)v.ulVal;
            else if (v.vt == VT_I2) res = v.iVal;
            else if (v.vt == VT_UI2) res = v.uiVal;
            else if (v.vt == VT_BSTR && v.bstrVal) res = _wtoi(v.bstrVal);
        }
        VariantClear(&v);
        obj->Release();
    }
    en->Release(); svc->Release(); loc->Release(); if (init) CoUninitialize();
    return res;
}

static std::wstring DiskType() {
    wchar_t w[MAX_PATH] = {};
    if (!GetWindowsDirectoryW(w, MAX_PATH)) return L"Unknown";
    std::wstring v = L"\\\\.\\  ";
    v[4] = w[0];
    v[5] = L':';
    HANDLE h = CreateFileW(v.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return L"Unknown";
    STORAGE_PROPERTY_QUERY sq = {};
    sq.PropertyId = StorageDeviceSeekPenaltyProperty;
    sq.QueryType = PropertyStandardQuery;
    DEVICE_SEEK_PENALTY_DESCRIPTOR dsd = {};
    DWORD r = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &sq, sizeof(sq), &dsd, sizeof(dsd), &r, 0);
    CloseHandle(h);
    if (!ok) return L"Unknown";
    return dsd.IncursSeekPenalty ? L"HDD" : L"SSD";
}

static bool ContainsAny(const std::wstring& s, const wchar_t* const* list, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s.find(list[i]) != std::wstring::npos) return true;
    }
    return false;
}

// Выбирает дискретную видеокарту, если она есть.
static std::wstring PickGpu() {
    const wchar_t* discreteKeys[] = {
        L"NVIDIA", L"GeForce", L"RTX", L"GTX", L"Quadro",
        L"Radeon", L"AMD", L"Arc", L"Intel(R) Arc"
    };

    std::vector<std::wstring> names = WMIStrings(
        L"SELECT Name FROM Win32_VideoController WHERE CurrentBitsPerPixel IS NOT NULL", L"Name");

    if (names.empty()) {
        names = WMIStrings(L"SELECT Name FROM Win32_VideoController", L"Name");
    }

    // 1) Дискретная карта
    for (const auto& n : names) {
        if (ContainsAny(n, discreteKeys, sizeof(discreteKeys) / sizeof(discreteKeys[0]))) {
            // исключаем виртуальные адаптеры
            if (n.find(L"Microsoft") == std::wstring::npos &&
                n.find(L"Basic Render") == std::wstring::npos &&
                n.find(L"Remote") == std::wstring::npos) {
                return n;
            }
        }
    }

    // 2) Любая, кроме базового адаптера Microsoft
    for (const auto& n : names) {
        if (n.find(L"Microsoft Basic") == std::wstring::npos &&
            n.find(L"Remote") == std::wstring::npos) {
            return n;
        }
    }

    // 3) Что есть
    if (!names.empty()) return names.front();
    return L"";
}

static std::wstring RamTypeFromSmbios(long v) {
    switch (v) {
    case 0x01: return L"Other";
    case 0x02: return L"Unknown";
    case 0x03: return L"DRAM";
    case 0x0F: return L"SDRAM";
    case 0x11: return L"SDRAM";
    case 0x12: return L"DDR";
    case 0x13: return L"DDR2";
    case 0x14: return L"DDR2 FB-DIMM";
    case 0x18: return L"DDR3";
    case 0x1A: return L"DDR4";
    case 0x22: return L"DDR5";
    case 0x23: return L"LPDDR5";
    case 0x1D: return L"LPDDR3";
    case 0x1E: return L"LPDDR4";
    default: return L"";
    }
}

#endif

HardwareInfo SystemInfo::GetInfo() {
    HardwareInfo info;
#ifdef _WIN32
    info.cpuName = WMIString(L"SELECT Name FROM Win32_Processor", L"Name");
    info.gpuName = PickGpu();
    info.motherboardName = WMIString(L"SELECT Product FROM Win32_BaseBoard", L"Product");

    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) info.ramGb = ms.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);

    long smbiosType = WMIInt(L"SELECT SMBIOSMemoryType FROM Win32_PhysicalMemory", L"SMBIOSMemoryType");
    info.ramType = RamTypeFromSmbios(smbiosType);
    if (info.ramType.empty()) {
        long memType = WMIInt(L"SELECT MemoryType FROM Win32_PhysicalMemory", L"MemoryType");
        info.ramType = RamTypeFromSmbios(memType);
    }

    long speed = WMIInt(L"SELECT Speed FROM Win32_PhysicalMemory", L"Speed");
    info.ramSpeed = (int)speed;

    info.systemDiskType = DiskType();
#endif
    if (info.cpuName.empty()) info.cpuName = L"Unknown";
    if (info.gpuName.empty()) info.gpuName = L"Unknown";
    if (info.motherboardName.empty()) info.motherboardName = L"Unknown";
    if (info.systemDiskType.empty()) info.systemDiskType = L"Unknown";
    return info;
}

} // namespace MutexTweaker
