#include "conversion.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace netcourse {

namespace {

std::string toMagnitudeBinary(long long value, int width) {
    std::string result(static_cast<std::size_t>(width), '0');
    for (int i = width - 1; i >= 0; --i) {
        result[static_cast<std::size_t>(i)] = static_cast<char>('0' + (value & 1LL));
        value >>= 1LL;
    }
    return result;
}

std::string invertBits(std::string bits) {
    for (char& bit : bits) {
        bit = (bit == '0') ? '1' : '0';
    }
    return bits;
}

std::string addOne(std::string bits) {
    for (int i = static_cast<int>(bits.size()) - 1; i >= 0; --i) {
        if (bits[static_cast<std::size_t>(i)] == '0') {
            bits[static_cast<std::size_t>(i)] = '1';
            return bits;
        }
        bits[static_cast<std::size_t>(i)] = '0';
    }
    return bits;
}

}  // namespace

ConversionResult convertToCodes(long long number, int bits) {
    if (bits < 2 || bits > 63) {
        throw std::runtime_error("Bit width must be in range 2..63");
    }

    const long long maxMagnitude = (1LL << (bits - 1)) - 1;
    if (number > maxMagnitude || number < -maxMagnitude) {
        throw std::runtime_error("Number does not fit selected bit width");
    }

    const auto magnitude = (number < 0) ? -number : number;
    const std::string magnitudeBits = toMagnitudeBinary(magnitude, bits - 1);

    if (number >= 0) {
        const std::string value = "0" + magnitudeBits;
        return ConversionResult{value, value};
    }

    const std::string reverse = "1" + invertBits(magnitudeBits);
    const std::string additional = addOne(reverse);
    return ConversionResult{reverse, additional};
}

}  // namespace netcourse
