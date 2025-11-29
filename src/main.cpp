#include "btree/btree.h"
#include "utils/bytes.h"
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    BNode b;
    b.set_header(BNODE_LEAF, 3);
    
    b.append_kv(0, 0, str_to_byte_vec("k1"), str_to_byte_vec("hi"));
    std::cout << b << std::endl;

    b.append_kv(1, 0, str_to_byte_vec("k2"), str_to_byte_vec("no"));
    std::cout << b << std::endl;

    b.append_kv(2, 0, str_to_byte_vec("k3"), str_to_byte_vec("yo"));
    std::cout << b << std::endl;

    auto key = b.get_key(1);
    for (uint8_t c : key) {
        std::cout << static_cast<char>(c);
    }
    auto val = b.get_val(1);
    for (uint8_t c : val) {
        std::cout << static_cast<char>(c);
    }
    


}
    