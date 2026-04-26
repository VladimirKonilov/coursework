#pragma once

#include <string>
#include <vector>

namespace netcourse {

// Унифицированное представление ответа сервера.
// ok      - признак успешности операции;
// code    - машинно-обрабатываемый код результата;
// message - текстовое пояснение для пользователя;
// fields  - дополнительные поля ответа, например вычисленные коды числа.
struct Response {
    bool ok;
    std::string code;
    std::string message;
    std::vector<std::string> fields;
};

// Разделяет входную строку протокола на отдельные поля.
std::vector<std::string> splitMessage(const std::string& line);
// Собирает поля в строку текстового прикладного протокола.
std::string joinMessage(const std::vector<std::string>& parts);
// Разбирает строковый ответ сервера в структуру Response.
Response parseResponse(const std::string& line);
// Проверяет, что токен безопасно передавать внутри текстового протокола.
bool isPrintableToken(const std::string& value);

}  // namespace netcourse
