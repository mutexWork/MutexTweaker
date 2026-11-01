#include "MutexTweaker/Utils.h"
#include <vector>

namespace MutexTweaker {
namespace Utils {

#ifdef _WIN32

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

bool SetRegDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName, DWORD data, REGSAM sam) {
    HKEY hKey = nullptr;
    DWORD disp = 0;
    LSTATUS st = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, sam, nullptr, &hKey, &disp);
    if (st != ERROR_SUCCESS) return false;
    st = RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(DWORD));
    RegCloseKey(hKey);
    return st == ERROR_SUCCESS;
}

bool SetRegString(HKEY root, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& data, REGSAM sam) {
    HKEY hKey = nullptr;
    DWORD disp = 0;
    LSTATUS st = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, sam, nullptr, &hKey, &disp);
    if (st != ERROR_SUCCESS) return false;
    st = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(data.c_str()), (DWORD)((data.size()+1)*sizeof(wchar_t)));
    RegCloseKey(hKey);
    return st == ERROR_SUCCESS;
}

bool DeleteRegValue(HKEY root, const std::wstring& subKey, const std::wstring& valueName, REGSAM sam) {
    HKEY hKey = nullptr;
    LSTATUS st = RegOpenKeyExW(root, subKey.c_str(), 0, sam, &hKey);
    if (st != ERROR_SUCCESS) return false;
    st = RegDeleteValueW(hKey, valueName.c_str());
    RegCloseKey(hKey);
    return st == ERROR_SUCCESS;
}

bool CreateRegKey(HKEY root, const std::wstring& subKey, REGSAM sam) {
    HKEY hKey = nullptr;
    DWORD disp = 0;
    LSTATUS st = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, sam, nullptr, &hKey, &disp);
    if (st != ERROR_SUCCESS) return false;
    RegCloseKey(hKey);
    return true;
}

bool KeyExists(HKEY root, const std::wstring& subKey) {
    HKEY hKey = nullptr;
    LSTATUS st = RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &hKey);
    if (st != ERROR_SUCCESS) return false;
    RegCloseKey(hKey);
    return true;
}

#endif

std::wstring ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring Trim(const std::wstring& s) {
    auto start = s.find_first_not_of(L" \t\n\r\f\v");
    if (start == std::wstring::npos) return L"";
    auto end = s.find_last_not_of(L" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

void Log(const std::wstring& msg) {
#ifdef _WIN32
    OutputDebugStringW((L"[MutexTweaker] " + msg + L"\n").c_str());
#endif
    (void)msg;
}

} // namespace Utils
} // namespace MutexTweaker
