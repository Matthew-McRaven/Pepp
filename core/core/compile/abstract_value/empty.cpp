#include "core/compile/abstract_value/empty.hpp"

pepp::ast::Empty::Empty() {}

pepp::ast::Empty::Empty(u8 size) : _size(size) {}

pepp::ast::Empty::Empty(const Empty &other) : _size(other._size) {}

pepp::ast::Empty::Empty(Empty &&other) noexcept { swap(*this, other); }

void pepp::ast::Empty::value(bits::span<u8> dest, bits::Order targetEndian) const noexcept {
  auto subspan = dest.subspan(0, stream_size());
  std::fill(subspan.begin(), subspan.end(), 0);
}
