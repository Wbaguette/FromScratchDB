#include "btree/btree.h"
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    BNode b;
    b.set_header(BNODE_LEAF, 3);
    

    b.append_kv(0, 0, std::vector<uint8_t>{'k', '1'}, std::vector<uint8_t>{'h', 'i'});
    std::cout << b << std::endl;

    b.append_kv(1, 0, std::vector<uint8_t>{'k', '2'}, std::vector<uint8_t>{'n', 'o'});
    std::cout << b << std::endl;

    b.append_kv(2, 0, std::vector<uint8_t>{'k', '3'}, std::vector<uint8_t>{'y', 'o'});
    std::cout << b << std::endl;

    auto x = b.get_key(1);
    for (uint8_t c : x) {
        std::cout << static_cast<char>(c);
    }

}
    