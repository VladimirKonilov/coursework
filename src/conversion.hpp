#pragma once

#include <string>

namespace netcourse {

// Структура результата вычисления кодов числа.
struct ConversionResult {
    // Обратный код числа.
    std::string reverse_code;
    // Дополнительный код числа.
    std::string additional_code;
};

// Вычисляет обратный и дополнительный код числа
// для заданной пользователем разрядности.
ConversionResult convertToCodes(long long number, int bits);

}  // namespace netcourse
