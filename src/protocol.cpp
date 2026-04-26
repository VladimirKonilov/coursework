#include "protocol.hpp"

#include <sstream>
#include <stdexcept>

namespace netcourse {

// Разделяет строку прикладного протокола на поля по символу '|'.
std::vector<std::string> splitMessage(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;
    for (char ch : line) {
        if (ch == '|') {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    parts.push_back(current);
    return parts;
}

// Собирает поля сообщения в одну строку прикладного протокола.
std::string joinMessage(const std::vector<std::string>& parts) {
    std::ostringstream builder;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            builder << '|';
        }
        builder << parts[i];
    }
    builder << '\n';
    return builder.str();
}

// Разбирает ответ сервера на успешный или ошибочный.
Response parseResponse(const std::string& line) {
    const auto parts = splitMessage(line);
    // Минимальный корректный ответ: тип, код, текстовое сообщение.
    if (parts.size() < 3) {
        throw std::runtime_error("Malformed response");
    }

    Response response{};
    if (parts[0] == "OK") {
        response.ok = true;
    } else if (parts[0] == "ERROR") {
        response.ok = false;
    } else {
        throw std::runtime_error("Unknown response type");
    }

    // Оставшиеся поля ответа интерпретируются как дополнительные данные,
    // например обратный и дополнительный код.
    response.code = parts[1];
    response.message = parts[2];
    if (parts.size() > 3) {
        response.fields.assign(parts.begin() + 3, parts.end());
    }
    return response;
}

// Запрещает символы, которые ломают формат текстового протокола.
bool isPrintableToken(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    for (unsigned char ch : value) {
        // Разделитель полей и символы конца строки запрещены,
        // иначе будет нарушен формат сообщений.
        if (ch == '|' || ch == '\n' || ch == '\r') {
            return false;
        }
        // Управляющие и не ASCII-символы также запрещаются для упрощения протокола.
        if (ch < 32 || ch > 126) {
            return false;
        }
    }
    return true;
}

}  // namespace netcourse
