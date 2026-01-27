#include "examples/examples.h"

int main() {
    const std::string db_path = "test.fsdb";

    basic_crud(db_path);
    restored_crud(db_path);
}