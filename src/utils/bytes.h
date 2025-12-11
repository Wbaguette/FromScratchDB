#pragma once
#include "../shared/bytevecview.h"
#include <cstdint>
#include <vector>
#include <string_view>

int lex_cmp_byte_vecs(ByteVecView a, ByteVecView b);

inline std::vector<uint8_t> str_to_byte_vec(std::string_view str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}
