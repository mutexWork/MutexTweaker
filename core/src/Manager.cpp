#include "MutexTweaker/Manager.h"
#include "MutexTweaker/SystemInfo.h"
#include "MutexTweaker/Tweaks/NetworkTweaks.h"
#include "MutexTweaker/Tweaks/PowerTweaks.h"
#include "MutexTweaker/Tweaks/ServicesTweaks.h"
#include "MutexTweaker/Tweaks/PrivacyTweaks.h"
#include "MutexTweaker/Tweaks/ExplorerTweaks.h"
#include "MutexTweaker/Tweaks/InputTweaks.h"
#include "MutexTweaker/Tweaks/StorageTweaks.h"
#include "MutexTweaker/Tweaks/LatencyTweaks.h"

#include <map>

namespace MutexTweaker {

struct Manager::Impl {
    std::vector<TweakInfo> list;
    std::map<std::string, std::function<TweakResult()>> funcs;
    HardwareInfo hw;

    void Build() {
        hw = SystemInfo::GetInfo();

        // ---------------- Сеть ----------------
        list.push_back({ "net_tcp_nagle", L"Отключить алгоритм Nagle",
                         L"TcpAckFrequency и TCPNoDelay = 1. Снижает задержку при передаче небольших пакетов.",
                         Category::Network, true, true });
        funcs["net_tcp_nagle"] = []() { return NetworkTweaks::DisableNagle(); };

        list.push_back({ "net_dns_flush", L"Сбросить DNS-кэш",
                         L"Очищает кэш сопоставлений доменных имён. Полезно после смены DNS-сервера.",
                         Category::Network, true, false });
        funcs["net_dns_flush"] = []() { return NetworkTweaks::FlushDns(); };

        list.push_back({ "net_throttling", L"Снять ограничение сети",
                         L"NetworkThrottlingIndex = 0xFFFFFFFF. Убирает троттлинг сетевого стека.",
                         Category::Network, true, true });
        funcs["net_throttling"] = []() { return NetworkTweaks::RemoveNetworkThrottling(); };

        list.push_back({ "net_delivery_opt", L"Отключить P2P-обновления",
                         L"Delivery Optimization не раздаёт обновления Windows другим устройствам.",
                         Category::Network, true, true });
        funcs["net_delivery_opt"] = []() { return NetworkTweaks::DisableDeliveryOptimization(); };

        // ---------------- Питание ----------------
        list.push_back({ "pow_highperf", L"Схема «Высокая производительность»",
                         L"Активирует стандартную схему электропитания SCHEME_MIN.",
                         Category::Power, true, true });
        funcs["pow_highperf"] = []() { return PowerTweaks::SetHighPerformance(); };

        list.push_back({ "pow_ultimate", L"Схема «Максимальная производительность»",
                         L"Создаёт и активирует Ultimate Performance через powercfg. Требует прав администратора.",
                         Category::Power, true, true });
        funcs["pow_ultimate"] = []() { return PowerTweaks::SetUltimatePerformance(); };

        // ---------------- Службы ----------------
        list.push_back({ "svc_sysmain", L"Отключить службу SysMain",
                         L"Ранее SuperFetch. Снижает фоновую нагрузку на диск и память.",
                         Category::Services, true, true });
        funcs["svc_sysmain"] = []() { return ServicesTweaks::DisableSysMain(); };

        list.push_back({ "svc_diagtrack", L"Отключить службу телеметрии",
                         L"DiagTrack: прекращает сбор и отправку диагностических данных.",
                         Category::Services, true, true });
        funcs["svc_diagtrack"] = []() { return ServicesTweaks::DisableDiagTrack(); };

        list.push_back({ "svc_wer", L"Отключить отчёты об ошибках",
                         L"WerSvc и Windows Error Reporting не создают и не отправляют отчёты.",
                         Category::Services, true, true });
        funcs["svc_wer"] = []() { return ServicesTweaks::DisableWerSvc(); };

        // ---------------- Конфиденциальность ----------------
        list.push_back({ "priv_telemetry_light", L"Ограничить телеметрию",
                         L"AllowTelemetry = 0 в политиках Windows. Отключает отправку диагностики.",
                         Category::Privacy, true, true });
        funcs["priv_telemetry_light"] = []() { return PrivacyTweaks::LimitTelemetrySafe(); };

        list.push_back({ "priv_gamedvr", L"Отключить Xbox Game DVR",
                         L"Отключает фоновую запись игр и захват приложений.",
                         Category::Privacy, true, true });
        funcs["priv_gamedvr"] = []() { return PrivacyTweaks::DisableGameDVR(); };

        list.push_back({ "priv_ads", L"Убрать рекламу и подсказки",
                         L"Отключает рекламный идентификатор, рекомендации в «Пуск» и на экране блокировки.",
                         Category::Privacy, true, true });
        funcs["priv_ads"] = []() { return PrivacyTweaks::LimitAdsAndSuggestions(); };

        list.push_back({ "priv_cortana", L"Отключить Cortana и веб-поиск",
                         L"Запрещает Cortana и выдачу веб-результатов в поиске Windows.",
                         Category::Privacy, true, true });
        funcs["priv_cortana"] = []() { return PrivacyTweaks::DisableCortana(); };

        list.push_back({ "priv_activity", L"Отключить журнал действий",
                         L"Windows не публикует и не отправляет историю активности.",
                         Category::Privacy, true, true });
        funcs["priv_activity"] = []() { return PrivacyTweaks::DisableActivityHistory(); };

        list.push_back({ "priv_feedback", L"Отключить запросы отзывов",
                         L"Windows больше не просит оставить отзыв и не показывает опросы.",
                         Category::Privacy, true, true });
        funcs["priv_feedback"] = []() { return PrivacyTweaks::DisableFeedback(); };

        // ---------------- Проводник ----------------
        list.push_back({ "exp_ext", L"Показывать расширения файлов",
                         L"HideFileExt = 0. Расширения снова видны в Проводнике.",
                         Category::Explorer, true, true });
        funcs["exp_ext"] = []() { return ExplorerTweaks::ShowFileExtensions(); };

        list.push_back({ "exp_hidden", L"Показывать скрытые файлы",
                         L"Hidden и ShowSuperHidden = 1 в параметрах Проводника.",
                         Category::Explorer, true, true });
        funcs["exp_hidden"] = []() { return ExplorerTweaks::ShowHiddenFiles(); };

        list.push_back({ "exp_launch", L"Открывать «Этот компьютер»",
                         L"Проводник при запуске открывает «Этот компьютер» вместо «Быстрого доступа».",
                         Category::Explorer, true, true });
        funcs["exp_launch"] = []() { return ExplorerTweaks::OpenThisPC(); };

        list.push_back({ "exp_restart_apps", L"Не восстанавливать приложения",
                         L"После перезагрузки Windows не открывает приложения заново.",
                         Category::Explorer, true, true });
        funcs["exp_restart_apps"] = []() { return ExplorerTweaks::DisableRestartApps(); };

        // ---------------- Ввод ----------------
        list.push_back({ "inp_mouse", L"Ускорить наведение мыши",
                         L"MouseHoverTime. Всплывающие подсказки появляются быстрее.",
                         Category::Input, true, true });
        funcs["inp_mouse"] = []() { return InputTweaks::TweakMouse(); };

        list.push_back({ "inp_keyboard", L"Уменьшить задержку клавиатуры",
                         L"KeyboardDelay = 0. Минимальная задержка повтора клавиш.",
                         Category::Input, true, true });
        funcs["inp_keyboard"] = []() { return InputTweaks::TweakKeyboard(); };

        list.push_back({ "inp_mouse_accel", L"Отключить ускорение мыши",
                         L"Курсор движется 1:1 без ускорения — точнее в играх и графике.",
                         Category::Input, true, true });
        funcs["inp_mouse_accel"] = []() { return InputTweaks::DisableMouseAcceleration(); };

        // ---------------- Хранилище ----------------
        list.push_back({ "stor_temp_user", L"Очистить папку %TEMP%",
                         L"Удаляет временные файлы текущего пользователя.",
                         Category::Storage, true, false });
        funcs["stor_temp_user"] = []() { return StorageTweaks::CleanUserTemp(); };

        list.push_back({ "stor_hibernate_off", L"Отключить гибернацию",
                         L"powercfg /h off: удаляет hiberfil.sys и освобождает место на диске.",
                         Category::Storage, true, true });
        funcs["stor_hibernate_off"] = []() { return StorageTweaks::DisableHibernation(); };

        list.push_back({ "stor_system_temp", L"Очистить системный Temp",
                         L"Удаляет временные файлы из C:\\Windows\\Temp.",
                         Category::Storage, true, false });
        funcs["stor_system_temp"] = []() { return StorageTweaks::CleanSystemTemp(); };

        // ---------------- Задержки ----------------
        list.push_back({ "lat_mmcss", L"Приоритет игр MMCSS",
                         L"SystemResponsiveness = 0 и высокий приоритет задач Games для отзывчивости.",
                         Category::Latency, true, true });
        funcs["lat_mmcss"] = []() { return LatencyTweaks::OptimizeSystemResponsiveness(); };
    }
};

Manager::Manager() : m_impl(std::make_unique<Impl>()) { m_impl->Build(); }
Manager::~Manager() = default;

const std::vector<TweakInfo>& Manager::GetAll() const { return m_impl->list; }

std::vector<TweakInfo> Manager::GetByCategory(Category cat) const {
    std::vector<TweakInfo> out;
    for (auto& t : m_impl->list) if (t.category == cat) out.push_back(t);
    return out;
}

std::vector<TweakInfo> Manager::GetSafeAll() const { return m_impl->list; }

TweakResult Manager::Apply(const std::string& id) {
    auto it = m_impl->funcs.find(id);
    if (it == m_impl->funcs.end()) {
        TweakResult r;
        r.success = false;
        r.message = L"Неизвестный твик";
        return r;
    }
    return it->second();
}

TweakResult Manager::ApplyByCategory(Category cat) {
    TweakResult total;
    total.success = true;
    total.rebootRequired = false;
    for (auto& t : m_impl->list) {
        if (t.category == cat) {
            auto r = Apply(t.id);
            if (!r.success) total.success = false;
            if (r.rebootRequired) total.rebootRequired = true;
        }
    }
    return total;
}

TweakResult Manager::ApplySafeAll() {
    TweakResult total;
    total.success = true;
    total.rebootRequired = false;
    for (auto& t : m_impl->list) {
        if (!t.isSafe) continue;
        auto r = Apply(t.id);
        if (!r.success) total.success = false;
        if (r.rebootRequired) total.rebootRequired = true;
    }
    return total;
}

HardwareInfo Manager::GetHardware() const { return m_impl->hw; }

} // namespace MutexTweaker
