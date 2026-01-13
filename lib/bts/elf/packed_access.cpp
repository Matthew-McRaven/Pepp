#include "./packed_access.hpp"

u32 pepp::bts::gnu_elf_hash(bits::span<const char> name) noexcept {
  u32 h = 5381;
  for (const auto &c : name) {
    h = (h << 5) + h + c;
  }
  return h;
}
u32 pepp::bts::gnu_elf_hash(std::string_view name) noexcept { return gnu_elf_hash(bits::span<const char>{name}); }
