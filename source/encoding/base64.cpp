#include "base64.h"

#include <stdexcept>

namespace {
constexpr char kEncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr int8_t kDecodeTable[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

template <size_t Count>
void decodeBlock(std::vector<uint8_t> &output, const std::string &input, size_t &offset) {
    uint8_t values[4] = {};
    for (size_t index = 0; index < Count; ++index) {
        const auto value = kDecodeTable[static_cast<uint8_t>(input[offset++])];
        if (value < 0) {
            throw std::invalid_argument("invalid Base64 character");
        }
        values[index] = static_cast<uint8_t>(value);
    }
    if constexpr (Count >= 2) {
        output.push_back(static_cast<uint8_t>((values[0] << 2) | (values[1] >> 4)));
    }
    if constexpr (Count >= 3) {
        output.push_back(static_cast<uint8_t>((values[1] << 4) | (values[2] >> 2)));
    }
    if constexpr (Count >= 4) {
        output.push_back(static_cast<uint8_t>((values[2] << 6) | values[3]));
    }
}
}  // namespace

std::string base64::encode(const std::vector<uint8_t> &data) {
    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);
    size_t index = 0;
    while (index + 3 <= data.size()) {
        encoded.push_back(kEncodeTable[data[index] >> 2]);
        encoded.push_back(kEncodeTable[((data[index] & 0x03) << 4) | (data[index + 1] >> 4)]);
        encoded.push_back(kEncodeTable[((data[index + 1] & 0x0F) << 2) | (data[index + 2] >> 6)]);
        encoded.push_back(kEncodeTable[data[index + 2] & 0x3F]);
        index += 3;
    }
    if (data.size() - index == 1) {
        encoded.push_back(kEncodeTable[data[index] >> 2]);
        encoded.push_back(kEncodeTable[(data[index] & 0x03) << 4]);
        encoded.append("==");
    } else if (data.size() - index == 2) {
        encoded.push_back(kEncodeTable[data[index] >> 2]);
        encoded.push_back(kEncodeTable[((data[index] & 0x03) << 4) | (data[index + 1] >> 4)]);
        encoded.push_back(kEncodeTable[(data[index + 1] & 0x0F) << 2]);
        encoded.push_back('=');
    }
    return encoded;
}

std::vector<uint8_t> base64::decode(const std::string &encoded) {
    if (encoded.size() % 4 != 0) {
        throw std::invalid_argument("invalid Base64 length");
    }
    if (encoded.empty()) {
        return {};
    }

    size_t padding = 0;
    if (encoded.back() == '=') {
        padding = 1;
        if (encoded.size() > 1 && encoded[encoded.size() - 2] == '=') {
            padding = 2;
        }
    }
    const size_t dataEnd = encoded.size() - padding;
    for (size_t index = 0; index < dataEnd; ++index) {
        if (kDecodeTable[static_cast<uint8_t>(encoded[index])] < 0) {
            throw std::invalid_argument("invalid Base64 character");
        }
    }
    for (size_t index = dataEnd; index < encoded.size(); ++index) {
        if (encoded[index] != '=') {
            throw std::invalid_argument("invalid Base64 padding");
        }
    }
    if (padding > 0 && encoded.size() < 4) {
        throw std::invalid_argument("invalid Base64 padding");
    }

    std::vector<uint8_t> output;
    output.reserve((encoded.size() / 4) * 3 - padding);
    size_t offset = 0;
    while (offset + 4 < encoded.size()) {
        decodeBlock<4>(output, encoded, offset);
    }
    const size_t finalDataChars = 4 - padding;
    if (padding == 0) {
        decodeBlock<4>(output, encoded, offset);
    } else {
        uint8_t values[4] = {};
        for (size_t index = 0; index < finalDataChars; ++index) {
            values[index] = static_cast<uint8_t>(kDecodeTable[static_cast<uint8_t>(encoded[offset++])]);
        }
        if (finalDataChars < 2) {
            throw std::invalid_argument("invalid Base64 padding");
        }
        output.push_back(static_cast<uint8_t>((values[0] << 2) | (values[1] >> 4)));
        if (finalDataChars == 3) {
            output.push_back(static_cast<uint8_t>((values[1] << 4) | (values[2] >> 2)));
        }
    }
    return output;
}