#include <cassert>
#include <string>

#include "../lib/database.h"
#include "examples.h"

// Using an already existing DB file
void restored_crud(const std::string& db_path) {
    auto kv = Database::KVStore(db_path, true);

    // Check that data we put there previously is still there
    auto name = kv.get("name");
    auto age = kv.get("age");
    CHECK_EQ(name, "Remus");
    CHECK_EQ(age, "67");

    // Check the previously deleted 'city' key is not there
    auto city = kv.get("city");
    CHECK(city.empty(), "city key was not deleted");

    // Add more data
    kv.insert("foo", "bar");
    auto foo = kv.get("foo");
    CHECK_EQ(foo, "bar");
}