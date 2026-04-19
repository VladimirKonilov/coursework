#pragma once

#include <string>
#include <vector>

namespace netcourse {

struct Response {
    bool ok;
    std::string code;
    std::string message;
    std::vector<std::string> fields;
};

std::vector<std::string> splitMessage(const std::string& line);
std::string joinMessage(const std::vector<std::string>& parts);
Response parseResponse(const std::string& line);
bool isPrintableToken(const std::string& value);

}  // namespace netcourse
