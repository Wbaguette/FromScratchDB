#pragma once
#include <cstdint>
#include <vector>
#include <string_view>
#include "../btree/btree.h"

int lex_cmp_byte_vecs(ByteVecView a, ByteVecView b);

inline std::vector<uint8_t> str_to_byte_vec(std::string_view str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}
