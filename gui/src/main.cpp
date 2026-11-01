#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cwchar>

#include "MutexTweaker/CAPI.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

#define IDC_LISTVIEW  1001
#define IDC_APPLY     1002
#define IDC_SELECTALL 1004
#define IDC_CLEARALL  1005
#define IDC_APPLYALL  1006
#define IDC_RESTORECHK 1007

#define WM_APP_RESTORE_DONE (WM_APP + 1)

// ---------- Палитра ----------
static const Color kBgTop     (255, 10, 10, 14);
static const Color kBgBottom  (255, 17, 17, 25);
static const Color kCard      (255, 22, 22, 30);
static const Color kCardBorder(255, 40, 40, 54);
static const Color kAccent    (255, 108, 92, 231);
static const Color kAccentHot (255, 126, 112, 255);
static const Color kTextMain  (255, 240, 240, 246);
static const Color kTextDim   (255, 140, 140, 158);
static const Color kOk        (255, 92, 200, 140);
static const Color kErr       (255, 232, 96, 96);

// ---------- Состояние ----------
static HWND  g_hMain = nullptr;
static HWND  g_list = nullptr;
static HWND  g_btnApply = nullptr;
static HWND  g_btnApplyAll = nullptr;
static HWND  g_btnAll = nullptr;
static HWND  g_btnNone = nullptr;
static HWND  g_chkRestore = nullptr;
static std::vector<std::string>  g_ids;
static std::vector<std::wstring> g_names;
static std::vector<bool>         g_reboot;
static std::wstring g_status = L"Отметьте нужные твики или нажмите «Применить всё»";
static Color        g_statusColor = kTextDim;

static std::wstring g_cpu, g_gpu, g_ram;
static bool         g_admin = false;
static bool         g_busy = false;
static bool         g_applyAll = false;
static bool         g_rpCreated = false;
static bool         g_createRp = true;
static volatile LONG g_rpResult = 0;
static wchar_t      g_rpError[512] = {};

static ULONG_PTR g_gdiToken = 0;
static HFONT g_hListFont = nullptr;
static Font* g_fTitle = nullptr;
static Font* g_fSub = nullptr;
static Font* g_fSection = nullptr;
static Font* g_fValue = nullptr;
static Font* g_fSmall = nullptr;
static Font* g_fButton = nullptr;

// ---------- Утилиты ----------
static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static bool IsAdmin()
{
    BOOL admin = FALSE;
    PSID grp = nullptr;
    SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&auth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &grp))
    {
        CheckTokenMembership(nullptr, grp, &admin);
        FreeSid(grp);
    }
    return admin == TRUE;
}

static bool HasElevatedMarker()
{
    return wcsstr(GetCommandLineW(), L"--elevated") != nullptr;
}

// Запускает копию приложения с правами администратора через UAC.
static bool RelaunchElevated()
{
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return false;

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.lpParameters = L"--elevated";
    sei.nShow = SW_SHOWMAXIMIZED;

    if (!ShellExecuteExW(&sei))
    {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED)
        {
            MessageBoxW(nullptr,
                L"Для изменения системных параметров нужны права администратора.\n\n"
                L"Запустите MutexTweaker ещё раз и подтвердите запрос UAC.",
                L"MutexTweaker", MB_OK | MB_ICONWARNING);
        }
        else
        {
            wchar_t msg[256];
            wsprintfW(msg, L"Не удалось получить права администратора (код %lu).",
                      (unsigned long)err);
            MessageBoxW(nullptr, msg, L"MutexTweaker", MB_OK | MB_ICONERROR);
        }
        return false;
    }

    if (sei.hProcess) CloseHandle(sei.hProcess);
    return true;
}

static void SetStatus(const std::wstring& text, Color c)
{
    g_status = text;
    g_statusColor = c;
    InvalidateRect(GetParent(g_list), nullptr, FALSE);
}

static void FillRound(Graphics& g, const RectF& r, float rad, Color fill, Color border)
{
    GraphicsPath path;
    float d = rad * 2.0f;
    path.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
    path.AddArc(r.X + r.Width - d, r.Y, d, d, 270.0f, 90.0f);
    path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0.0f, 90.0f);
    path.AddArc(r.X, r.Y + r.Height - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();

    SolidBrush b(fill);
    g.FillPath(&b, &path);

    if (border.GetA() > 0)
    {
        Pen p(border, 1.0f);
        g.DrawPath(&p, &path);
    }
}

static void DrawStr(Graphics& g, const std::wstring& s, Font* f, Color c, float x, float y)
{
    SolidBrush b(c);
    g.DrawString(s.c_str(), (INT)s.size(), f, PointF(x, y), &b);
}

// Обрезает строку по ширине, добавляя многоточие.
static std::wstring FitText(Graphics& g, const std::wstring& s, Font* f, float maxW)
{
    if (s.empty()) return s;

    RectF box;
    g.MeasureString(s.c_str(), (INT)s.size(), f, PointF(0, 0), &box);
    if (box.Width <= maxW) return s;

    std::wstring cut = s;
    while (!cut.empty())
    {
        cut.pop_back();
        std::wstring probe = cut + L"…";
        g.MeasureString(probe.c_str(), (INT)probe.size(), f, PointF(0, 0), &box);
        if (box.Width <= maxW) return probe;
    }
    return L"…";
}

// Приводит название процессора к аккуратному виду.
static std::wstring PrettyCpu(const std::wstring& src)
{
    std::wstring s = src;
    auto replaceAll = [](std::wstring& str, const std::wstring& from, const std::wstring& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::wstring::npos)
        {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replaceAll(s, L"(R)", L"");
    replaceAll(s, L"(TM)", L"");
    replaceAll(s, L"  ", L" ");
    replaceAll(s, L" CPU @ ", L" @ ");
    // убираем пробел перед @
    size_t p = s.find(L" @");
    if (p != std::wstring::npos && p > 0 && s[p - 1] == L' ') s.erase(p - 1, 1);
    // обрезаем хвост после частоты
    size_t at = s.find(L" @ ");
    if (at != std::wstring::npos)
    {
        size_t sp = s.find(L' ', at + 3);
        if (sp != std::wstring::npos) s = s.substr(0, sp);
    }
    return s;
}

// Приводит название видеокарты к аккуратному виду.
static std::wstring PrettyGpu(const std::wstring& src)
{
    std::wstring s = src;
    auto replaceAll = [](std::wstring& str, const std::wstring& from, const std::wstring& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::wstring::npos)
        {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replaceAll(s, L"(R)", L"");
    replaceAll(s, L"(TM)", L"");
    replaceAll(s, L"AMD ", L"");
    replaceAll(s, L" Series", L"");
    replaceAll(s, L"  ", L" ");
    while (!s.empty() && s.front() == L' ') s.erase(s.begin());
    while (!s.empty() && s.back() == L' ') s.pop_back();
    return s;
}

static const wchar_t* CategoryName(int cat)
{
    switch (cat)
    {
    case 0: return L"Сеть";
    case 1: return L"Питание";
    case 2: return L"Службы";
    case 3: return L"Приватность";
    case 4: return L"Проводник";
    case 5: return L"Ввод";
    case 6: return L"Хранилище";
    case 7: return L"Задержки";
    default: return L"Прочее";
    }
}

// ---------- Отрисовка кнопок ----------
static void DrawButton(const DRAWITEMSTRUCT* dis)
{
    Graphics g(dis->hDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    RectF r((REAL)dis->rcItem.left, (REAL)dis->rcItem.top,
            (REAL)(dis->rcItem.right - dis->rcItem.left),
            (REAL)(dis->rcItem.bottom - dis->rcItem.top));

    bool primary = (dis->CtlID == IDC_APPLY || dis->CtlID == IDC_APPLYALL);
    bool pressed = (dis->itemState & ODS_SELECTED) != 0;

    Color fill = primary ? (pressed ? Color(255, 92, 78, 205) : kAccent) : Color(255, 30, 30, 40);
    Color border = primary ? Color(0, 0, 0, 0) : kCardBorder;
    if (!primary && pressed) fill = Color(255, 38, 38, 50);

    FillRound(g, r, 9.0f, fill, border);

    wchar_t text[128] = {};
    GetWindowTextW(dis->hwndItem, text, 128);
    SolidBrush b(primary ? Color(255, 255, 255, 255) : Color(255, 205, 205, 218));
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(text, -1, g_fButton, r, &fmt, &b);
}

// ---------- Отрисовка чекбокса точки восстановления ----------
static void DrawRestoreCheck(const DRAWITEMSTRUCT* dis)
{
    Graphics g(dis->hDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    int w = dis->rcItem.right - dis->rcItem.left;
    int h = dis->rcItem.bottom - dis->rcItem.top;

    // Фон под контролом совпадает с нижней частью градиента окна.
    SolidBrush bg(Color(255, 17, 17, 25));
    g.FillRectangle(&bg, 0, 0, w, h);

    bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    bool checked = g_createRp;

    const float box = 17.0f;
    float by = (h - box) / 2.0f;
    RectF boxRect(1.0f, by, box, box);

    Color boxFill = checked ? kAccent : Color(255, 30, 30, 42);
    Color boxBorder = checked ? kAccentHot : kCardBorder;
    if (disabled)
    {
        boxFill = checked ? Color(255, 74, 66, 130) : Color(255, 26, 26, 34);
        boxBorder = Color(255, 48, 48, 60);
    }
    FillRound(g, boxRect, 4.0f, boxFill, boxBorder);

    if (checked)
    {
        Pen pen(Color(255, 255, 255, 255), 2.0f);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        PointF p1(boxRect.X + 4.0f, boxRect.Y + 8.5f);
        PointF p2(boxRect.X + 7.5f, boxRect.Y + 12.0f);
        PointF p3(boxRect.X + 13.0f, boxRect.Y + 5.0f);
        g.DrawLine(&pen, p1, p2);
        g.DrawLine(&pen, p2, p3);
    }

    wchar_t text[128] = {};
    GetWindowTextW(dis->hwndItem, text, 128);
    Color textColor = disabled ? Color(255, 120, 120, 136) : Color(255, 255, 255, 255);
    SolidBrush tb(textColor);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentNear);
    fmt.SetLineAlignment(StringAlignmentCenter);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    RectF tr(boxRect.X + box + 8.0f, 0.0f,
             (REAL)w - (boxRect.X + box + 8.0f) - 2.0f, (REAL)h);
    g.DrawString(text, -1, g_fSub, tr, &fmt, &tb);
}

// ---------- Отрисовка окна ----------
static void PaintWindow(HWND hWnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // фон
    LinearGradientBrush bg(Rect(0, 0, W, H), kBgTop, kBgBottom, LinearGradientModeVertical);
    g.FillRectangle(&bg, 0, 0, W, H);

    // акцентное свечение сверху
    {
        RectF glow(W * 0.18f, -160.0f, W * 0.64f, 260.0f);
        GraphicsPath gp;
        gp.AddEllipse(glow);
        PathGradientBrush pb(&gp);
        pb.SetCenterColor(Color(46, 108, 92, 231));
        Color surround[] = { Color(0, 10, 10, 14) };
        int cnt = 1;
        pb.SetSurroundColors(surround, &cnt);
        g.FillPath(&pb, &gp);
    }

    // ---- Заголовок ----
    DrawStr(g, L"MutexTweaker", g_fTitle, kTextMain, 30.0f, 20.0f);
    DrawStr(g, L"Тонкая настройка Windows без лишнего шума", g_fSub, kTextDim, 31.0f, 60.0f);

    // ---- Карточка системы ----
    RectF card(30.0f, 96.0f, (REAL)W - 60.0f, 108.0f);
    FillRound(g, card, 14.0f, kCard, kCardBorder);
    DrawStr(g, L"КОНФИГУРАЦИЯ", g_fSection, kTextDim, card.X + 22.0f, card.Y + 16.0f);

    struct Col { const wchar_t* label; const std::wstring* value; float frac; };
    Col cols[3] = {
        { L"ПРОЦЕССОР",           &g_cpu, 0.40f },
        { L"ВИДЕОКАРТА",          &g_gpu, 0.32f },
        { L"ОПЕРАТИВНАЯ ПАМЯТЬ",  &g_ram, 0.28f }
    };

    float innerW = card.Width - 44.0f;
    float colX = card.X + 22.0f;
    for (int i = 0; i < 3; ++i)
    {
        float colW = innerW * cols[i].frac;
        DrawStr(g, cols[i].label, g_fSmall, kTextDim, colX, card.Y + 44.0f);

        std::wstring v = *cols[i].value;
        if (v.empty()) v = L"—";
        DrawStr(g, FitText(g, v, g_fValue, colW - 24.0f), g_fValue, kTextMain, colX, card.Y + 64.0f);
        colX += colW;
    }

    // ---- Заголовок секции ----
    DrawStr(g, L"ТВИКИ", g_fSection, kTextDim, 32.0f, 226.0f);
    {
        wchar_t info[64];
        wsprintfW(info, L"%d доступно", (int)g_ids.size());
        int len = lstrlenW(info);
        (void)len;
        DrawStr(g, info, g_fSmall, Color(255, 100, 100, 118), 96.0f, 228.0f);
    }

    // ---- Статус ----
    DrawStr(g, g_status, g_fSmall, g_statusColor, 32.0f, (float)H - 52.0f);

    // разделительная линия снизу
    Pen line(Color(255, 32, 32, 44), 1.0f);
    g.DrawLine(&line, 30.0f, (REAL)H - 96.0f, (REAL)W - 30.0f, (REAL)H - 96.0f);
}

// ---------- Загрузка твиков из ядра ----------
static void LoadTweaks()
{
    g_ids.clear();
    g_names.clear();
    g_reboot.clear();

    int count = WO_GetTweakCount();
    for (int i = 0; i < count; ++i)
    {
        const char* id = WO_GetTweakId(i);
        const wchar_t* name = WO_GetTweakName(i);
        const wchar_t* desc = WO_GetTweakDesc(i);
        int cat = WO_GetTweakCategory(i);

        if (!id || !*id) continue;

        std::wstring wname = (name && *name) ? name : Utf8ToWide(id);
        std::wstring wdesc = (desc && *desc) ? desc : L"";

        LVITEMW it = {};
        it.mask = LVIF_TEXT;
        it.iItem = (int)g_ids.size();
        it.pszText = const_cast<LPWSTR>(wname.c_str());
        ListView_InsertItem(g_list, &it);

        ListView_SetItemText(g_list, it.iItem, 1, const_cast<LPWSTR>(CategoryName(cat)));
        ListView_SetItemText(g_list, it.iItem, 2, const_cast<LPWSTR>(wdesc.c_str()));
        ListView_SetCheckState(g_list, it.iItem, FALSE);

        g_ids.push_back(id);
        g_names.push_back(wname);
        // твики, требующие перезагрузки
        std::string sid = id;
        bool rb = (sid == "net_tcp_nagle" || sid == "net_throttling" ||
                   sid == "priv_telemetry_light");
        g_reboot.push_back(rb);
    }
}

static void LoadSystemInfo()
{
    WO_Init();

    const wchar_t* cpu = WO_GetCpuName();
    const wchar_t* gpu = WO_GetGpuName();
    const wchar_t* ramType = WO_GetRamType();
    double ram = WO_GetRamGb();
    int ramSpeed = WO_GetRamSpeed();

    g_cpu = PrettyCpu((cpu && *cpu) ? cpu : L"Неизвестно");
    g_gpu = PrettyGpu((gpu && *gpu) ? gpu : L"Неизвестно");

    wchar_t buf[128];
    // wsprintfW не поддерживает %f, поэтому округляем вручную.
    if (ram >= 1.0)
    {
        int whole = (int)(ram + 0.5);
        wsprintfW(buf, L"%d ГБ", whole);
    }
    else
    {
        int tenths = (int)(ram * 10.0 + 0.5);
        wsprintfW(buf, L"%d.%d ГБ", tenths / 10, tenths % 10);
    }

    g_ram = buf;
    if (ramType && *ramType && lstrcmpW(ramType, L"Unknown") != 0)
    {
        g_ram += L" ";
        g_ram += ramType;
    }
    if (ramSpeed > 0)
    {
        wsprintfW(buf, L" · %d МГц", ramSpeed);
        g_ram += buf;
    }
}

// ---------- Применение твиков ----------
static void ApplySelected(HWND hWnd, bool all);

static void SetBusy(bool busy)
{
    g_busy = busy;
    EnableWindow(g_btnApply, !busy);
    EnableWindow(g_btnApplyAll, !busy);
    EnableWindow(g_btnAll, !busy);
    EnableWindow(g_btnNone, !busy);
    EnableWindow(g_chkRestore, !busy);
    EnableWindow(g_list, !busy);
}

// Точка восстановления создаётся в фоновом потоке: это может занять до минуты.
static DWORD WINAPI RestorePointThread(LPVOID)
{
    wchar_t err[512] = {};
    int r = WO_CreateRestorePoint(L"MutexTweaker: перед применением твиков", err, 512);
    lstrcpynW(g_rpError, err, 512);
    InterlockedExchange((volatile LONG*)&g_rpResult, r);
    PostMessageW(g_hMain, WM_APP_RESTORE_DONE, 0, 0);
    return 0;
}

// Проверяет выбор, спрашивает подтверждение и при необходимости создаёт точку восстановления.
static void StartApply(HWND hWnd, bool all)
{
    if (g_busy) return;

    int n = ListView_GetItemCount(g_list);
    int selected = 0;
    bool reboot = false;

    for (int i = 0; i < n; ++i)
    {
        bool checked = all ? true : (ListView_GetCheckState(g_list, i) != FALSE);
        if (all) ListView_SetCheckState(g_list, i, TRUE);
        if (!checked) continue;
        if (i >= (int)g_ids.size()) continue;
        selected++;
        if (i < (int)g_reboot.size() && g_reboot[i]) reboot = true;
    }

    if (selected == 0)
    {
        SetStatus(L"Ничего не выбрано — отметьте хотя бы один твик", Color(255, 220, 170, 80));
        return;
    }

    if (!g_admin)
    {
        SetStatus(L"Нужны права администратора: перезапустите приложение от имени администратора", kErr);
        MessageBoxW(hWnd,
            L"Приложение запущено без прав администратора.\n"
            L"Большинство твиков не смогут изменить систему.\n\n"
            L"Закройте программу и запустите её от имени администратора.",
            L"Недостаточно прав", MB_OK | MB_ICONWARNING);
        return;
    }

    if (g_chkRestore)
        InvalidateRect(g_chkRestore, nullptr, TRUE);

    wchar_t msg[900];
    if (g_createRp)
    {
        if (reboot)
        {
            wsprintfW(msg,
                L"Будет создана точка восстановления системы.\n"
                L"Это может занять до минуты.\n\n"
                L"Затем будет применено твиков: %d.\n"
                L"Часть изменений вступит в силу только после перезагрузки.\n\n"
                L"Продолжить?", selected);
        }
        else
        {
            wsprintfW(msg,
                L"Будет создана точка восстановления системы.\n"
                L"Это может занять до минуты.\n\n"
                L"Затем будет применено твиков: %d.\n\n"
                L"Продолжить?", selected);
        }
    }
    else
    {
        if (reboot)
        {
            wsprintfW(msg,
                L"Точка восстановления создаваться НЕ будет.\n\n"
                L"Будет применено твиков: %d.\n"
                L"Часть изменений вступит в силу только после перезагрузки.\n\n"
                L"Продолжить?", selected);
        }
        else
        {
            wsprintfW(msg,
                L"Точка восстановления создаваться НЕ будет.\n\n"
                L"Будет применено твиков: %d.\n\n"
                L"Продолжить?", selected);
        }
    }

    if (MessageBoxW(hWnd, msg, L"Применение твиков", MB_YESNO | MB_ICONWARNING) != IDYES)
    {
        SetStatus(L"Применение отменено", kTextDim);
        return;
    }

    g_applyAll = all;
    g_rpCreated = false;
    g_rpError[0] = L'\0';
    InterlockedExchange((volatile LONG*)&g_rpResult, 0);

    if (!g_createRp)
    {
        ApplySelected(hWnd, all);
        return;
    }

    SetBusy(true);
    SetStatus(L"Создаю точку восстановления системы… это может занять до минуты",
              Color(255, 220, 170, 80));
    UpdateWindow(hWnd);

    HANDLE th = CreateThread(nullptr, 0, RestorePointThread, nullptr, 0, nullptr);
    if (!th)
    {
        SetBusy(false);
        SetStatus(L"Не удалось запустить создание точки восстановления", kErr);
        MessageBoxW(hWnd,
            L"Не удалось запустить создание точки восстановления.\n"
            L"Твики не применены.",
            L"Ошибка", MB_OK | MB_ICONERROR);
    }
    else
    {
        CloseHandle(th);
    }
}

static void ApplySelected(HWND hWnd, bool all)
{
    int n = ListView_GetItemCount(g_list);
    int selected = 0, applied = 0, failed = 0;
    bool reboot = false;

    for (int i = 0; i < n; ++i)
    {
        bool checked = all ? true : (ListView_GetCheckState(g_list, i) != FALSE);
        if (all) ListView_SetCheckState(g_list, i, TRUE);
        if (!checked) continue;
        if (i >= (int)g_ids.size()) continue;
        selected++;
        if (i < (int)g_reboot.size() && g_reboot[i]) reboot = true;
    }

    if (selected == 0) return;
    if (!g_admin) return;

    for (int i = 0; i < n; ++i)
    {
        bool checked = all ? true : (ListView_GetCheckState(g_list, i) != FALSE);
        if (!checked) continue;
        if (i >= (int)g_ids.size()) continue;

        if (i < (int)g_names.size())
        {
            SetStatus(L"Применяю: " + g_names[i] + L"…", kTextDim);
            UpdateWindow(hWnd);
        }

        int r = WO_ApplyTweak(g_ids[i].c_str());
        if (r == 1)
        {
            applied++;
            ListView_SetCheckState(g_list, i, FALSE);
        }
        else
        {
            failed++;
        }
    }

    wchar_t buf[320];
    if (failed == 0)
    {
        wsprintfW(buf, L"Готово: успешно применено %d из %d", applied, selected);
        if (g_rpCreated) lstrcatW(buf, L" · точка восстановления создана");
        SetStatus(buf, kOk);
    }
    else
    {
        wsprintfW(buf, L"Применено %d из %d, ошибок: %d", applied, selected, failed);
        if (g_rpCreated) lstrcatW(buf, L" · точка восстановления создана");
        SetStatus(buf, kErr);
    }

    SetBusy(false);

    if (reboot && applied > 0)
    {
        int res = MessageBoxW(hWnd,
            L"Изменения применены.\n\nПерезагрузить компьютер сейчас?",
            L"Требуется перезагрузка", MB_YESNO | MB_ICONQUESTION);
        if (res == IDYES)
        {
            HANDLE tok = nullptr;
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
            {
                TOKEN_PRIVILEGES tp = {};
                tp.PrivilegeCount = 1;
                LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid);
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                AdjustTokenPrivileges(tok, FALSE, &tp, 0, nullptr, nullptr);
                CloseHandle(tok);
            }
            ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_SOFTWARE);
        }
    }
}

// ---------- Процедура окна ----------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icc);

        HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

        g_admin = IsAdmin();

        g_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
            0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_LISTVIEW, hi, nullptr);

        ListView_SetExtendedListViewStyle(g_list,
            LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

        if (g_hListFont)
            SendMessageW(g_list, WM_SETFONT, (WPARAM)g_hListFont, TRUE);

        // высота строки списка
        HIMAGELIST il = ImageList_Create(1, 30, ILC_COLOR32, 1, 1);
        if (il) ListView_SetImageList(g_list, il, LVSIL_SMALL);

        ListView_SetTextColor(g_list, RGB(236, 236, 244));
        ListView_SetTextBkColor(g_list, RGB(22, 22, 30));
        ListView_SetBkColor(g_list, RGB(22, 22, 30));

        LVCOLUMNW c = {};
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)L"Твик";
        c.cx = 300;
        ListView_InsertColumn(g_list, 0, &c);

        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)L"Категория";
        c.cx = 150;
        ListView_InsertColumn(g_list, 1, &c);

        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)L"Описание";
        c.cx = 380;
        ListView_InsertColumn(g_list, 2, &c);

        LoadTweaks();
        LoadSystemInfo();

        g_btnAll = CreateWindowExW(0, L"BUTTON", L"Выбрать все",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_SELECTALL, hi, nullptr);

        g_btnNone = CreateWindowExW(0, L"BUTTON", L"Снять все",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_CLEARALL, hi, nullptr);

        g_btnApply = CreateWindowExW(0, L"BUTTON", L"Применить выбранное",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_APPLY, hi, nullptr);

        g_btnApplyAll = CreateWindowExW(0, L"BUTTON", L"Применить всё",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_APPLYALL, hi, nullptr);

        g_chkRestore = CreateWindowExW(0, L"BUTTON", L"Сделать точку восстановления",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 10, 10, hWnd, (HMENU)(ULONG_PTR)IDC_RESTORECHK, hi, nullptr);
        if (g_hListFont)
            SendMessageW(g_chkRestore, WM_SETFONT, (WPARAM)g_hListFont, TRUE);
    }

    case WM_SIZE:
    {
        int W = LOWORD(lParam), H = HIWORD(lParam);
        int left = 30, right = W - 30;
        int listTop = 250;
        int listH = H - 250 - 120;

        if (g_list)
            MoveWindow(g_list, left, listTop, right - left, listH, TRUE);

        if (g_list)
        {
            RECT lr;
            GetClientRect(g_list, &lr);
            int descW = lr.right - 300 - 150 - 8;
            if (descW < 180) descW = 180;
            ListView_SetColumnWidth(g_list, 2, descW);
        }

        if (g_btnAll)
            MoveWindow(g_btnAll, left, listTop + listH + 18, 130, 38, TRUE);
        if (g_btnNone)
            MoveWindow(g_btnNone, left + 142, listTop + listH + 18, 130, 38, TRUE);
        if (g_btnApplyAll)
            MoveWindow(g_btnApplyAll, right - 390, listTop + listH + 18, 170, 38, TRUE);
        if (g_btnApply)
            MoveWindow(g_btnApply, right - 210, listTop + listH + 18, 210, 38, TRUE);
        if (g_chkRestore)
            MoveWindow(g_chkRestore, left, listTop + listH + 68, 420, 22, TRUE);

        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        PaintWindow(hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT* dis = (const DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == IDC_RESTORECHK)
            DrawRestoreCheck(dis);
        else
            DrawButton(dis);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(240, 240, 246));
        SetBkColor(hdc, RGB(22, 22, 30));
        static HBRUSH br = CreateSolidBrush(RGB(22, 22, 30));
        return (LRESULT)br;
    }

    case WM_NOTIFY:
    {
        LPNMHDR hdr = (LPNMHDR)lParam;
        if (g_list && hdr->hwndFrom == ListView_GetHeader(g_list) &&
            hdr->code == NM_CUSTOMDRAW)
        {
            LPNMCUSTOMDRAW cd = (LPNMCUSTOMDRAW)lParam;
            if (cd->dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (cd->dwDrawStage == CDDS_ITEMPREPAINT)
            {
                static HBRUSH hb = CreateSolidBrush(RGB(22, 22, 30));
                FillRect(cd->hdc, &cd->rc, hb);
                SetTextColor(cd->hdc, RGB(150, 150, 172));
                SetBkColor(cd->hdc, RGB(22, 22, 30));
                return CDRF_NEWFONT;
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (id == IDC_SELECTALL || id == IDC_CLEARALL)
        {
            int n = ListView_GetItemCount(g_list);
            for (int i = 0; i < n; ++i)
                ListView_SetCheckState(g_list, i, id == IDC_SELECTALL);
            return 0;
        }

        if (id == IDC_RESTORECHK && HIWORD(wParam) == BN_CLICKED)
        {
            g_createRp = !g_createRp;
            InvalidateRect(g_chkRestore, nullptr, TRUE);
            return 0;
        }

        if (id == IDC_APPLY || id == IDC_APPLYALL)
        {
            StartApply(hWnd, id == IDC_APPLYALL);
            return 0;
        }
        break;
    }

    case WM_APP_RESTORE_DONE:
    {
        SetBusy(false);

        bool ok = (InterlockedCompareExchange((volatile LONG*)&g_rpResult, 0, 0) == 1);
        if (!ok)
        {
            std::wstring text = L"Не удалось создать точку восстановления системы.";
            if (g_rpError[0])
            {
                text += L"\n\nПричина: ";
                text += g_rpError;
            }
            text += L"\n\nПрименить твики без точки восстановления?";

            if (MessageBoxW(hWnd, text.c_str(), L"Точка восстановления не создана",
                            MB_YESNO | MB_ICONERROR | MB_DEFBUTTON2) != IDYES)
            {
                SetStatus(L"Применение отменено: точка восстановления не создана", kErr);
                return 0;
            }
            g_rpCreated = false;
        }
        else
        {
            g_rpCreated = true;
        }

        ApplySelected(hWnd, g_applyAll);
        return 0;
    }

    case WM_DESTROY:
        delete g_fTitle;
        delete g_fSub;
        delete g_fSection;
        delete g_fValue;
        delete g_fSmall;
        delete g_fButton;
        g_fTitle = g_fSub = g_fSection = g_fValue = g_fSmall = g_fButton = nullptr;

        if (g_gdiToken) GdiplusShutdown(g_gdiToken);
        if (g_hListFont) DeleteObject(g_hListFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    (void)nCmdShow;

    // Стартуем без повышенных прав и сами запрашиваем UAC.
    // Это надёжно работает при запуске из установщика, ярлыка и сторонних лаунчеров.
    if (!IsAdmin() && !HasElevatedMarker())
    {
        if (RelaunchElevated()) return 0;
        return 1;
    }

    // DPI awareness (если доступно)
    typedef BOOL (WINAPI* SetCtxFn)(HANDLE);
    if (HMODULE u32 = GetModuleHandleW(L"user32.dll"))
    {
        SetCtxFn setCtx = (SetCtxFn)GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (setCtx) setCtx((HANDLE)-4); // PER_MONITOR_AWARE_V2
        else SetProcessDPIAware();
    }

    GdiplusStartupInput gsi;
    if (GdiplusStartup(&g_gdiToken, &gsi, nullptr) != Ok)
        return 0;

    FontFamily family(L"Segoe UI");
    g_fTitle   = new Font(&family, 26.0f, FontStyleBold,   UnitPixel);
    g_fSub     = new Font(&family, 13.0f, FontStyleRegular, UnitPixel);
    g_fSection = new Font(&family, 10.0f, FontStyleBold,   UnitPixel);
    g_fValue   = new Font(&family, 15.0f, FontStyleRegular, UnitPixel);
    g_fSmall   = new Font(&family, 11.0f, FontStyleRegular, UnitPixel);
    g_fButton  = new Font(&family, 13.0f, FontStyleBold,   UnitPixel);

    g_hListFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"MutexTweakerUI";
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm       = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                        16, 16, LR_DEFAULTCOLOR);

    RegisterClassExW(&wc);

    // Окно открывается развёрнутым на весь экран (рабочая область монитора).
    RECT work = { 0, 0, 0, 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int w = work.right - work.left;
    int h = work.bottom - work.top;

    HWND hWnd = CreateWindowExW(0, L"MutexTweakerUI", L"MutexTweaker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        work.left, work.top, w, h,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 0;

    g_hMain = hWnd;

    ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
