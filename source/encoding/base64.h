#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace base64 {
std::string encode(const std::vector<uint8_t> &data);
std::vector<uint8_t> decode(const std::string &encoded);
}  // namespace base64