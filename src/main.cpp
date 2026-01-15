#include <cassert>
#include <string>

#include "lib/database.h"

int main() {
    const std::string db_path = "test.fsdb";
    auto kv = Database::KVStore(db_path, false);

    // Put some data in there
    kv.insert("name", "Romulus");
    kv.insert("age", "67");
    kv.insert("city", "Rome");
    auto name = kv.get("name");
    auto age = kv.get("age");
    auto city = kv.get("city");
    assert(name == "Romulus");
    assert(age == "67");
    assert(city == "Rome");

    // Update some data
    kv.insert("name", "Remus");
    auto updated_name = kv.get("name");
    assert(updated_name == "Remus");

    // Delete some data
    bool deleted = kv.remove("city");
    auto check = kv.get("city");
    assert(check.empty());
}