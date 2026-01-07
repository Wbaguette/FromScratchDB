#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

#include "../shared/views.h"

int lex_cmp_byte_vecs(ByteVecView a, ByteVecView b);
void print_byte_vec_view(ByteVecView a);

inline std::vector<uint8_t> str_to_byte_vec(std::string_view str) {
    return {str.begin(), str.end()};
}
