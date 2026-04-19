#pragma once

#include <string>

namespace netcourse {

struct ConversionResult {
    std::string reverse_code;
    std::string additional_code;
};

ConversionResult convertToCodes(long long number, int bits);

}  // namespace netcourse
