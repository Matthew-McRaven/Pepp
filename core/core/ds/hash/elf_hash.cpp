#include "elf_hash.hpp"

u32 pepp::elf_hash(bits::span<const char> name) noexcept {
  u32 h = 0, g;
  for (const auto &c : name) {
    h = (h << 4) + c;
    if ((g = h & 0xf0000000)) h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

u32 pepp::elf_hash(std::string_view name) noexcept { return elf_hash(bits::span<const char>{name}); }
