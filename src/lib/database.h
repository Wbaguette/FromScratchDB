#pragma once

#include <memory>

#include "../disk/kv.h"
#include "../utils/bytes.h"

// High level API
namespace Database {

struct KVStore {
   private:
    std::unique_ptr<KV> m_KV;

   public:
    explicit KVStore(const std::string& fp, bool restore_from_file)
        : m_KV(std::make_unique<KV>(fp, restore_from_file)) {}
    KVStore(const KVStore& other) = delete;
    ~KVStore() = default;
    KVStore& operator=(const KVStore& other) = delete;
    // Move constructor
    KVStore(KVStore&& other) noexcept = delete;
    // Move assignment
    KVStore& operator=(KVStore&& other) noexcept = delete;

    void insert(const std::string& key, const std::string& value) const {
        m_KV->set(str_to_byte_vec(key), str_to_byte_vec(value));
    }

    [[nodiscard]] std::string_view get(const std::string& key) const {
        return m_KV->get(str_to_byte_vec(key)).value_or("");
    }

    [[nodiscard]] bool remove(const std::string& key) const {
        return m_KV->del(str_to_byte_vec(key));
    };
};
}  // namespace Database