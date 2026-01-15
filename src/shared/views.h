#pragma once
#include <cstdint>
#include <span>
#include <vector>

using ByteVecView = std::span<const uint8_t>;
using MutableByteVecView = std::span<uint8_t>;
using MmapView = std::vector<std::span<const uint8_t>>;
using PageView = std::vector<std::span<const uint8_t>>;
