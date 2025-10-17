#include "btree.h"
#include <iostream>

int main() {
    // CHECKING IF READ_LE ACTUALLY WORKS
    BNode b;

    b.setHeader(2, 5);

    std::cout << "btype=" << b.btype() << std::endl;
    std::cout << "nkeys=" << b.nkeys() << std::endl; 

    
}