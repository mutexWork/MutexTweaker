#pragma once

#include "Types.h"
#include <string>

namespace MutexTweaker {

// Создание точки восстановления Windows перед изменением системы.
class SystemRestore {
public:
    // Создаёт точку восстановления. При необходимости включает защиту системы.
    // При ошибке заполняет error текстом для пользователя.
    static bool Create(const std::wstring& description, std::wstring* error = nullptr);
};

} // namespace MutexTweaker
