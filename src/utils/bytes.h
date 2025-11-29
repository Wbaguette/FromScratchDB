#pragma once
#include <cstdint>
#include <vector>
#include <string_view>

inline std::vector<uint8_t> str_to_byte_vec(std::string_view str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}
