#pragma once
#include "../shared/views.h"
#include <cstdint>
#include <vector>
#include <string_view>

int lex_cmp_byte_vecs(ByteVecView a, ByteVecView b);

inline std::vector<uint8_t> str_to_byte_vec(std::string_view str) {
  return {str.begin(), str.end()};
}
