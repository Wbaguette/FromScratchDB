#include "btree.h"
#include <iostream>

int main() {
    BNode b;
    std::cout << b.data.capacity() << std::endl;

    b.set_header(BNODE_LEAF, 2);

    std::cout << "btype=" << b.btype() << std::endl;
    std::cout << "nkeys=" << b.nkeys() << std::endl; 

    
}