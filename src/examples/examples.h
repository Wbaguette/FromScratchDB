#pragma once

#include <string>
#define CHECK(con, message) \
    if (!(con)) throw std::runtime_error(message)
#define CHECK_EQ(actual, expected) \
    if (actual != expected)        \
    throw std::runtime_error(std::format("expected: {}, got: {}", expected, actual))

void basic_crud(const std::string& db_path);
void restored_crud(const std::string& db_path);
void bulk_crud(const std::string& db_path);
