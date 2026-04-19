#include "protocol.hpp"

#include <sstream>
#include <stdexcept>

namespace netcourse {

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

Response parseResponse(const std::string& line) {
    const auto parts = splitMessage(line);
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

    response.code = parts[1];
    response.message = parts[2];
    if (parts.size() > 3) {
        response.fields.assign(parts.begin() + 3, parts.end());
    }
    return response;
}

bool isPrintableToken(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    for (unsigned char ch : value) {
        if (ch == '|' || ch == '\n' || ch == '\r') {
            return false;
        }
        if (ch < 32 || ch > 126) {
            return false;
        }
    }
    return true;
}

}  // namespace netcourse
