#include "bytes.h"

#include <algorithm>
#include <format>
#include <iostream>

// Compare returns an integer comparing two byte slices lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
// if length of a is < b, then return -1 because a < b
// if length of a is > b, then return +1 because a > b
int lex_cmp_byte_vecs(ByteVecView a, ByteVecView b) {
    size_t n = std::min(a.size(), b.size());
    const int rc = std::memcmp(a.data(), b.data(), n);

    if (rc != 0) {
        return rc;
    }

    if (a.size() < b.size()) {
        return -1;
    }
    if (a.size() > b.size()) {
        return 1;
    }

    return 0;
}

void print_byte_vec_view(ByteVecView a) {
    if (a.empty()) {
        std::cout << "empty, data not found\n";
        return;
    }

    for (const auto c : a) {
        if (c >= 32 && c < 127) {
            std::cout << static_cast<char>(c);
        } else {
            std::cout << std::format("\\x{:02x}", c);
        }
    }
    std::cout << "\n";
}
