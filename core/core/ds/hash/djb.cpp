#include "djb.hpp"

// As specified by: https://blogs.oracle.com/solaris/gnu-hash-elf-sections-v2
u64 pepp::djb(bits::span<const char> name) noexcept {
  u64 h = 5381;
  for (const auto &c : name) {
    h = (h << 5) + h + c;
  }
  return h;
}

u64 pepp::djb(std::string_view name) noexcept { return djb(bits::span<const char>{name}); }
