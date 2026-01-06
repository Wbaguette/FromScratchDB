#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "disk/kv.h"
#include "utils/bytes.h"

int main() {
    try {
        const std::string db_path = "test.fsdb";

        KV db(db_path);
        db.init();

        std::cout << "Database opened: " << db_path << "\n\n";

        std::cout << "Inserting data...\n";
        db.set(str_to_byte_vec("name"), str_to_byte_vec("Alice"));
        db.set(str_to_byte_vec("age"), str_to_byte_vec("25"));
        db.set(str_to_byte_vec("city"), str_to_byte_vec("Boston"));
        std::cout << "  Inserted 3 key-value pairs\n\n";

        // Retrieve and display data
        std::cout << "Reading data...\n";
        auto name = db.get(str_to_byte_vec("name"));
        auto age = db.get(str_to_byte_vec("age"));
        auto city = db.get(str_to_byte_vec("city"));

        if (!name.empty()) {
            std::cout << "  name: ";
            for (uint8_t c : name) {
                std::cout << static_cast<char>(c);
            }
            std::cout << "\n";
        }

        if (!age.empty()) {
            std::cout << "  age: ";
            for (uint8_t c : age) {
                std::cout << static_cast<char>(c);
            }
            std::cout << "\n";
        }

        if (!city.empty()) {
            std::cout << "  city: ";
            for (uint8_t c : city) {
                std::cout << static_cast<char>(c);
            }
            std::cout << "\n";
        }

        // Update a value
        std::cout << "\nUpdating age to 26...\n";
        db.set(str_to_byte_vec("age"), str_to_byte_vec("26"));

        auto updated_age = db.get(str_to_byte_vec("age"));
        if (!updated_age.empty()) {
            std::cout << "  new age: ";
            for (uint8_t c : updated_age) {
                std::cout << static_cast<char>(c);
            }
            std::cout << "\n";
        }

        // Delete a key
        std::cout << "\nDeleting city...\n";
        bool deleted = db.del(str_to_byte_vec("city"));
        std::cout << "  Delete " << (deleted ? "successful" : "failed") << "\n";

        auto check = db.get(str_to_byte_vec("city"));
        std::cout << "  city is " << (check.empty() ? "gone" : "still there") << "\n";

        close(db.m_Fd);
        std::cout << "\nDatabase closed successfully!\n";
    } catch (std::exception& e) {
        std::cout << e.what();
    }
}