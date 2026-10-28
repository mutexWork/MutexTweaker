#pragma once

#include "Types.h"

#ifdef _WIN32
#define WO_API __declspec(dllexport)
#else
#define WO_API
#endif

extern "C" {

WO_API void WO_Init();
WO_API int WO_GetTweakCount();
WO_API const char* WO_GetTweakId(int index);
WO_API const wchar_t* WO_GetTweakName(int index);
WO_API const wchar_t* WO_GetTweakDesc(int index);
WO_API int WO_GetTweakCategory(int index);
WO_API int WO_ApplyTweak(const char* id);
WO_API int WO_ApplyAllSafe();
WO_API int WO_ApplyByCategory(int cat);

// Создаёт точку восстановления. 1 — успех, 0 — ошибка (текст в errorBuf).
WO_API int WO_CreateRestorePoint(const wchar_t* description,
                                 wchar_t* errorBuf, int errorBufLen);

WO_API const wchar_t* WO_GetCpuName();
WO_API const wchar_t* WO_GetGpuName();
WO_API double WO_GetRamGb();
WO_API double WO_GetVramGb();
WO_API const wchar_t* WO_GetRamType();
WO_API int WO_GetRamSpeed();
WO_API const wchar_t* WO_GetDiskType();
}
