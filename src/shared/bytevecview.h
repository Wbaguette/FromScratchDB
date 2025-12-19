#pragma once
#include <cstdint>
#include <span>
#include <vector>

// Non-owning, readonly, vector of bytes
typedef std::span<const uint8_t> ByteVecView;
typedef std::vector<std::span<const uint8_t>> MmapView;
typedef std::vector<std::span<const uint8_t>> PageView;
