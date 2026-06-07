#pragma once
#include <string_view>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"

namespace pepp {
// Used as the hash for gnu's .gnu.hash section, and is generally useful for non-cryptographic purposes.
u64 djb(bits::span<const char>) noexcept;
u64 djb(std::string_view) noexcept;

template <std::integral I> u64 djb(I value) {
  return djb(bits::span<const char>{reinterpret_cast<const char *>(&value), sizeof(I)});
}
} // namespace pepp