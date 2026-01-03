#include <cstdint>
#include <cstring>
#include <iostream>

#include "btree/bnode.h"
#include "utils/bytell_hash_map.hpp"
#include "utils/bytes.h"

int main() {
    // auto m = ska::bytell_hash_map<int, int>{};

    // m.emplace(1, 10);

    // // auto x = m.at(2);
    // auto found = m.find(1);
    // if (found == m.end())
    //     std::cout << "Not in map" << std::endl;

    BNode b;
    b.set_header(BNODE_LEAF, 3);

    node_append_kv(b, 0, 0, str_to_byte_vec("k1"), str_to_byte_vec("hi"));
    std::cout << b << "\n";

    node_append_kv(b, 1, 0, str_to_byte_vec("k2"), str_to_byte_vec("no"));
    std::cout << b << "\n";

    node_append_kv(b, 2, 0, str_to_byte_vec("k3"), str_to_byte_vec("yo"));
    std::cout << b << "\n";

    auto key = b.get_key(1);
    for (uint8_t c : key) {
        std::cout << static_cast<char>(c);
    }
    auto val = b.get_val(1);
    for (uint8_t c : val) {
        std::cout << static_cast<char>(c);
    }
}
