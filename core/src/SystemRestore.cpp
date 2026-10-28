#include "MutexTweaker/SystemRestore.h"
#include "MutexTweaker/Utils.h"

#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {

// Значения из srrestoreptapi.h (заголовок есть не во всех SDK, поэтому дублируем).
const DWORD kBeginSystemChange = 100;
const DWORD kModifySettings    = 12;
const int   kMaxDesc           = 256;

struct RestorePointInfoW
{
    DWORD dwEventType;
    DWORD dwRestorePtType;
    INT64 llSequenceNumber;
    wchar_t szDescription[kMaxDesc];
};

struct StateMgrStatus
{
    DWORD nStatus;
    INT64 llSequenceNumber;
};

typedef BOOL (WINAPI* SRSetRestorePointW_t)(RestorePointInfoW*, StateMgrStatus*);

bool EnablePrivilege(const wchar_t* name)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    {
        return false;
    }

    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    bool ok = false;
    if (LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid))
    {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        SetLastError(ERROR_SUCCESS);
        if (AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr))
            ok = (GetLastError() == ERROR_SUCCESS);
    }

    CloseHandle(token);
    return ok;
}

std::wstring SystemDriveRoot()
{
    wchar_t winDir[MAX_PATH] = {};
    if (!GetWindowsDirectoryW(winDir, MAX_PATH)) return L"C:\\";

    // "C:\Windows" -> "C:\"
    if (winDir[0] && winDir[1] == L':')
    {
        wchar_t root[4] = { winDir[0], L':', L'\\', L'\0' };
        return root;
    }
    return L"C:\\";
}

// Запускает консольную команду без окна и ждёт её завершения.
DWORD RunHidden(const std::wstring& cmdLine, DWORD timeoutMs)
{
    std::wstring mutableCmd = cmdLine;
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, &mutableCmd[0], nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        return (DWORD)-1;
    }

    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD code = (DWORD)-1;
    if (wait == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code;
}

// Включает защиту системы на системном диске (нужно для создания точки).
bool EnableSystemProtection()
{
    wchar_t sysDir[MAX_PATH] = {};
    if (!GetSystemDirectoryW(sysDir, MAX_PATH)) return false;

    std::wstring ps = std::wstring(sysDir) + L"\\WindowsPowerShell\\v1.0\\powershell.exe";
    if (GetFileAttributesW(ps.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    // Кавычки вокруг -Command и отсутствие обратного слэша перед ними —
    // чтобы не сломать разбор командной строки.
    std::wstring cmd =
        L"\"" + ps + L"\" -NoProfile -NonInteractive -ExecutionPolicy Bypass "
        L"-Command \"Enable-ComputerRestore -Drive ([System.IO.Path]::GetPathRoot($env:SystemRoot))\"";
    return RunHidden(cmd, 90000) == 0;
}

std::wstring DescribeStatus(DWORD status)
{
    wchar_t buf[256];
    switch (status)
    {
    case 0x80070005:
        return L"нет прав администратора";
    case 0x80070422:
        return L"служба восстановления системы отключена";
    case 0x80070424:
        return L"служба восстановления системы не установлена";
    default:
        wsprintfW(buf, L"код 0x%08lX", (unsigned long)status);
        return buf;
    }
}
} // namespace
#endif // _WIN32

namespace MutexTweaker {

bool SystemRestore::Create(const std::wstring& description, std::wstring* error)
{
    auto fail = [error](const std::wstring& msg) -> bool {
        if (error) *error = msg;
        return false;
    };

#ifndef _WIN32
    return fail(L"восстановление системы доступно только в Windows");
#else
    if (!Utils::IsRunningAsAdmin())
        return fail(L"нужны права администратора");

    EnablePrivilege(SE_BACKUP_NAME);
    EnablePrivilege(SE_RESTORE_NAME);

    // Отключаем суточное ограничение на создание точек (если оно включено).
    Utils::SetRegDword(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore",
        L"SystemRestorePointCreationFrequency", 0);

    HMODULE sr = LoadLibraryW(L"srclient.dll");
    if (!sr)
        return fail(L"не загрузилась библиотека srclient.dll");

    SRSetRestorePointW_t createPoint =
        (SRSetRestorePointW_t)GetProcAddress(sr, "SRSetRestorePointW");
    if (!createPoint)
    {
        FreeLibrary(sr);
        return fail(L"система не поддерживает SRSetRestorePointW");
    }

    std::wstring desc = description;
    if (desc.size() > 200) desc.resize(200);
    if (desc.empty()) desc = L"MutexTweaker";

    DWORD lastStatus = 0;
    bool protectionEnabled = false;

    // Первая попытка — как есть. Если защита выключена, включаем её и повторяем.
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        RestorePointInfoW info = {};
        info.dwEventType = kBeginSystemChange;
        info.dwRestorePtType = kModifySettings;
        info.llSequenceNumber = 0;

        wcsncpy_s(info.szDescription, kMaxDesc, desc.c_str(), _TRUNCATE);

        StateMgrStatus status = {};
        BOOL ok = createPoint(&info, &status);

        if (ok && status.nStatus == 0 && status.llSequenceNumber != 0)
        {
            FreeLibrary(sr);
            return true;
        }

        lastStatus = (status.nStatus != 0) ? status.nStatus : (DWORD)GetLastError();

        if (attempt == 0)
        {
            // Скорее всего защита системы выключена — включаем и ждём службу.
            protectionEnabled = EnableSystemProtection();
            Sleep(protectionEnabled ? 4000 : 1500);
        }
        else
        {
            Sleep(2000);
        }
    }

    FreeLibrary(sr);

    std::wstring msg = DescribeStatus(lastStatus);
    if (protectionEnabled)
        msg += L"; защита системы была включена, попробуйте ещё раз";
    return fail(msg);
#endif
}

} // namespace MutexTweaker
