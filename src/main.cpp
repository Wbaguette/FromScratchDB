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
    auto name = db.get(str_to_byte_vec("name"));
    auto age = db.get(str_to_byte_vec("age"));
    auto city = db.get(str_to_byte_vec("city"));

    print_byte_vec_view(name);
    print_byte_vec_view(age);
    print_byte_vec_view(city);

    // Update some data
    db.set(str_to_byte_vec("age"), str_to_byte_vec("26"));
    auto updated_age = db.get(str_to_byte_vec("age"));
    // Verify updated data
    print_byte_vec_view(updated_age);

    // Delete some data
    bool deleted = db.del(str_to_byte_vec("city"));
    // Make sure data got deleted
    std::cout << "Delete " << (deleted ? "successful" : "failed") << "\n";
    auto check = db.get(str_to_byte_vec("city"));
    print_byte_vec_view(check);
}