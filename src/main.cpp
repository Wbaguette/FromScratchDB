#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "disk/kv.h"
#include "utils/bytes.h"

int main() {
    const std::string db_path = "test.fsdb";

    KV db(db_path, false);
    db.init();

    std::cout << "Database opened: " << db_path << "\n\n";

    // Put some data in there
    db.set(str_to_byte_vec("name"), str_to_byte_vec("Alice"));
    db.set(str_to_byte_vec("age"), str_to_byte_vec("25"));
    db.set(str_to_byte_vec("city"), str_to_byte_vec("Boston"));

    // Make sure the data is there
    auto name = db.get(str_to_byte_vec("name")).value_or("Not found");
    auto age = db.get(str_to_byte_vec("age")).value_or("Not found");
    auto city = db.get(str_to_byte_vec("city")).value_or("Not found");

    std::cout << name << "\n";
    std::cout << age << "\n";
    std::cout << city << "\n";

    // Update some data
    db.set(str_to_byte_vec("age"), str_to_byte_vec("26"));
    auto updated_age = db.get(str_to_byte_vec("age")).value_or("Not found");
    // Verify updated data
    std::cout << updated_age << "\n";

    // Delete some data
    bool deleted = db.del(str_to_byte_vec("city"));
    // Make sure data got deleted
    std::cout << "Delete " << (deleted ? "successful" : "failed") << "\n";
    auto check = db.get(str_to_byte_vec("city")).value_or("Not found");
    std::cout << check << "\n";
}