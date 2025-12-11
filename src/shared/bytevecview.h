#pragma once
#include <cstdint>
#include <span>

// Non-owning, readonly, vector of bytes
typedef std::span<const uint8_t> ByteVecView;