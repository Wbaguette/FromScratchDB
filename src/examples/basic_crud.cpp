#include <string>

#include "../lib/database.h"
#include "examples.h"

// Creating a new database file and doing basic CRUD
void basic_crud(const std::string& db_path) {
    auto kv = Database::KVStore(db_path, false);

    // Put some data in there
    kv.insert("name", "Romulus");
    kv.insert("age", "67");
    kv.insert("city", "Rome");
    auto name = kv.get("name");
    auto age = kv.get("age");
    auto city = kv.get("city");

    CHECK_EQ(name, "Romulus");
    CHECK_EQ(age, "67");
    CHECK_EQ(city, "Rome");

    // Update some data
    kv.insert("name", "Remus");
    auto updated_name = kv.get("name");
    CHECK_EQ(updated_name, "Remus");

    // Delete some data
    bool deleted = kv.remove("city");
    (void)deleted;
    auto check = kv.get("city");
    CHECK(check.empty(), "city key was not deleted");

    // Delete a key that doesn't exist
    bool deleted2 = kv.remove("kappa");
    (void)deleted2;
    // False is returned because since the key didn't exist, no deletion occured
    CHECK(deleted2 == false, "deletion somehow occured");
}