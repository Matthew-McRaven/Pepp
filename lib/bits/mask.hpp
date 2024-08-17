#pragma once
#include <cstdint>

namespace bits {
// Convert a byte count to a mask.
// e.g., 1=>0xFF
// 2=> 0xFFFF
uint64_t mask(uint8_t byteCount);
} // namespace bits
