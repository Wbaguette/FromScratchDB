#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../lib/database.h"
#include "examples.h"

// Creating a new database file and doing bulk CRUD
void bulk_crud(const std::string& db_path) {
    auto kv = Database::KVStore(db_path, false);

    // Put some data in there
    for (int i = 0; i < 1000; i++) {
        // auto key = std::to_string(i);
        // auto value = std::to_string(i);
        // std::cout << key << " " << value << "\n";
        kv.insert(std::to_string(i), "bar");
    }

    // Verify
    for (int i = 0; i < 1000; i++) {
        // auto key = std::to_string(i);
        // auto actual = std::to_string(i);
        std::string actual = "bar";
        auto value = kv.get(std::to_string(i));

        if (value != actual) {
            std::cout << value << " " << actual << "\n";
            throw std::runtime_error("whoopsies");
        }
    }

    // // Update some data
    // for (int i = 0; i < 100000; i++) {
    //     auto key = std::to_string(i);
    //     auto value = std::to_string(i);
    //     kv.insert(key, value);
    // }

    // // Delete some data
    // bool deleted = kv.remove("city");
    // (void)deleted;
    // auto check = kv.get("city");
    // assert(check.empty());

    // // Delete a key that doesn't exist
    // bool deleted2 = kv.remove("kappa");
    // (void)deleted2;
    // // False is returned because since the key didn't exist, no deletion occured
    // assert(deleted2 == false);
}