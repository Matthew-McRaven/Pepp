#pragma once
#include <string_view>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"

namespace pepp {

// As specified by: https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#hash
u32 elf_hash(bits::span<const char>) noexcept;
u32 elf_hash(std::string_view) noexcept;

template <std::integral I> u32 elf_hash(I value) {
  return elf_hash(bits::span<const char>{reinterpret_cast<const char *>(&value), sizeof(I)});
}
} // namespace pepp