#include "btree/bnode.h"
#include "utils/bytes.h"
#include <cstdint>
#include <cstring>
#include <iostream>

// TODO: static casts on stuff because fuck this language 

int main() {
    BNode b;
    b.set_header(BNODE_LEAF, 3);
    
    // node_append_kv(b, 0, 0, str_to_byte_vec("k1"), ByteVecView val)


    node_append_kv(b, 0, 0, str_to_byte_vec("k1"), str_to_byte_vec("hi"));
    std::cout << b << std::endl;

    node_append_kv(b, 1, 0, str_to_byte_vec("k2"), str_to_byte_vec("no"));
    std::cout << b << std::endl;

    node_append_kv(b, 2, 0, str_to_byte_vec("k3"), str_to_byte_vec("yo"));
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
    