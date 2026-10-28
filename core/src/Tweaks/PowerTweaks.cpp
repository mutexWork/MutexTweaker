#include "MutexTweaker/Tweaks/PowerTweaks.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <powrprof.h>
#include <string>
#pragma comment(lib, "powrprof.lib")
#endif

namespace MutexTweaker {

#ifdef _WIN32

static const GUID kHighPerf =
    { 0x8c5e7fda, 0xe8bf, 0x4a96, { 0x9a, 0x85, 0xa6, 0xe2, 0x3a, 0x8c, 0x63, 0x5c } };

static const GUID kUltimatePerf =
    { 0xe9a42b02, 0xd5df, 0x448d, { 0xaa, 0x00, 0x03, 0xf1, 0x47, 0x49, 0xeb, 0x61 } };

// Встречающийся в некоторых сборках вариант GUID схемы Ultimate Performance.
static const GUID kUltimatePerfAlt =
    { 0xe9a42b02, 0xd5df, 0x448d, { 0xaa, 0x00, 0x03, 0x9f, 0x49, 0x65, 0x7e, 0x90 } };

static const wchar_t* kUltimateStr    = L"e9a42b02-d5df-448d-aa00-03f14749eb61";
static const wchar_t* kUltimateAltStr = L"e9a42b02-d5df-448d-aa00-039f49657e90";
static const wchar_t* kHighPerfStr = L"8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";

// Runs powercfg.exe with the given arguments. Optionally captures stdout.
static bool RunPowercfg(const std::wstring& args, std::wstring* output = nullptr) {
    wchar_t sysDir[MAX_PATH] = {};
    if (!GetSystemDirectoryW(sysDir, MAX_PATH)) return false;

    std::wstring exe = std::wstring(sysDir) + L"\\powercfg.exe";
    std::wstring cmdLine = L"\"" + exe + L"\" " + args;

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE readPipe = nullptr, writePipe = nullptr;
    bool usePipe = (output != nullptr);
    if (usePipe) {
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (usePipe) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;
        si.hStdInput = nullptr;
    }

    PROCESS_INFORMATION pi = {};
    std::wstring mutableCmd = cmdLine;
    BOOL started = CreateProcessW(nullptr, &mutableCmd[0], nullptr, nullptr,
                                  usePipe ? TRUE : FALSE, CREATE_NO_WINDOW,
                                  nullptr, nullptr, &si, &pi);

    if (writePipe) CloseHandle(writePipe);

    if (!started) {
        if (readPipe) CloseHandle(readPipe);
        return false;
    }

    std::wstring captured;
    if (usePipe) {
        // powercfg writes ANSI/UTF-16 depending on locale; read bytes then decode.
        std::string raw;
        char buf[1024];
        DWORD read = 0;
        while (ReadFile(readPipe, buf, sizeof(buf), &read, nullptr) && read > 0) {
            raw.append(buf, read);
        }
        CloseHandle(readPipe);

        if (!raw.empty()) {
            // Try UTF-16LE first
            bool looksUtf16 = (raw.size() >= 2 && (unsigned char)raw[1] == 0);
            if (looksUtf16) {
                captured.assign(reinterpret_cast<const wchar_t*>(raw.data()), raw.size() / 2);
            } else {
                int n = MultiByteToWideChar(CP_OEMCP, 0, raw.data(), (int)raw.size(), nullptr, 0);
                if (n > 0) {
                    std::wstring w(n, L'\0');
                    MultiByteToWideChar(CP_OEMCP, 0, raw.data(), (int)raw.size(), &w[0], n);
                    captured = w;
                }
            }
        }
    }

    WaitForSingleObject(pi.hProcess, 25000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (output) *output = captured;
    return exitCode == 0;
}

// Extracts the first GUID-looking token from text.
static bool FindGuid(const std::wstring& text, std::wstring& out) {
    const size_t need = 36;
    if (text.size() < need) return false;
    for (size_t i = 0; i + need <= text.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < need; ++j) {
            wchar_t c = text[i + j];
            if (j == 8 || j == 13 || j == 18 || j == 23) {
                if (c != L'-') { ok = false; break; }
            } else {
                bool hex = (c >= L'0' && c <= L'9') ||
                           (c >= L'a' && c <= L'f') ||
                           (c >= L'A' && c <= L'F');
                if (!hex) { ok = false; break; }
            }
        }
        if (ok) {
            out = text.substr(i, need);
            return true;
        }
    }
    return false;
}

static bool ActivateByGuid(const GUID& g) {
    return PowerSetActiveScheme(nullptr, &g) == ERROR_SUCCESS;
}

#endif // _WIN32

TweakResult PowerTweaks::SetHighPerformance() {
    TweakResult r;
#ifdef _WIN32
    r.success = ActivateByGuid(kHighPerf);
    if (!r.success) {
        r.success = RunPowercfg(std::wstring(L"-setactive ") + kHighPerfStr);
    }
    r.rebootRequired = false;
#endif
    return r;
}

TweakResult PowerTweaks::SetUltimatePerformance() {
    TweakResult r;
#ifdef _WIN32
    // 1) Схема уже существует — просто активируем (проверяем оба известных GUID).
    r.success = ActivateByGuid(kUltimatePerf);
    if (r.success) { r.rebootRequired = false; return r; }

    r.success = ActivateByGuid(kUltimatePerfAlt);
    if (r.success) { r.rebootRequired = false; return r; }

    // 2) Схемы нет — создаём копию скрытой схемы Ultimate Performance.
    const wchar_t* templates[2] = { kUltimateStr, kUltimateAltStr };
    for (int t = 0; t < 2; ++t) {
        std::wstring out;
        if (RunPowercfg(std::wstring(L"-duplicatescheme ") + templates[t], &out)) {
            std::wstring newGuid;
            if (FindGuid(out, newGuid)) {
                if (RunPowercfg(L"-setactive " + newGuid)) {
                    r.success = true;
                    r.rebootRequired = false;
                    return r;
                }
            }
            // Копия могла создаться, но GUID не распознан — пробуем активировать шаблон.
            if (RunPowercfg(std::wstring(L"-setactive ") + templates[t])) {
                r.success = true;
                r.rebootRequired = false;
                return r;
            }
        }
    }

    // 3) Крайний случай — «Высокая производительность».
    r.success = SetHighPerformance().success;
    r.rebootRequired = false;
#endif
    return r;
}

} // namespace MutexTweaker
