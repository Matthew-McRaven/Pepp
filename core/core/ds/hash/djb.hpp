#pragma once
#include <string_view>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"

namespace pepp {
// Basis for hash for gnu's .gnu.hash section, and is generally useful for non-cryptographic purposes.
u64 djb(bits::span<const char>) noexcept;
u64 djb(std::string_view) noexcept;

// Actual width specified by .gnu.hash
inline u32 djb32(bits::span<const char> name) noexcept { return static_cast<u32>(djb(name)); }
inline u32 djb32(std::string_view name) noexcept { return static_cast<u32>(djb(name)); }

template <std::integral I> u64 djb(I value) {
  return djb(bits::span<const char>{reinterpret_cast<const char *>(&value), sizeof(I)});
}
} // namespace pepp