#pragma once

#include "Types.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace MutexTweaker {
namespace Utils {

#ifdef _WIN32
bool IsRunningAsAdmin();
bool SetRegDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName, DWORD data, REGSAM sam = KEY_SET_VALUE | KEY_WOW64_64KEY);
bool SetRegString(HKEY root, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& data, REGSAM sam = KEY_SET_VALUE | KEY_WOW64_64KEY);
bool DeleteRegValue(HKEY root, const std::wstring& subKey, const std::wstring& valueName, REGSAM sam = KEY_SET_VALUE | KEY_WOW64_64KEY);
bool CreateRegKey(HKEY root, const std::wstring& subKey, REGSAM sam = KEY_WRITE | KEY_WOW64_64KEY);
bool KeyExists(HKEY root, const std::wstring& subKey);
#endif

std::wstring ToWide(const std::string& str);
std::string ToUtf8(const std::wstring& wstr);
std::wstring Trim(const std::wstring& s);
void Log(const std::wstring& msg);

} // namespace Utils
} // namespace MutexTweaker
