#include "packed_ops.hpp"

u64 pepp::bts::size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) noexcept {
  u64 ret = 0;
  for (const auto &item : layout) ret = std::max(ret, item.offset + item.data.size());
  return ret;
}

void pepp::bts::write(std::span<u8> out, const std::vector<LayoutItem> &layout) {
  for (const auto &item : layout) {
    if (item.offset + item.data.size() > out.size())
      throw std::runtime_error("Elf::write: layout item exceeds output size");
    std::span<u8> chunk = out.subspan(item.offset, item.data.size());
    bits::memcpy<u8, u8>(chunk, {item.data});
  }
}
