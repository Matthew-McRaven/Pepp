#include "./log2.hpp"
#include <bit>
quint8 bits::ceil_log2(quint64 value) {
  if (value == 0)
    throw std::logic_error("Must be non-0");
  quint64 ceil = std::bit_ceil(value);
  return sizeof(value) * 8 - std::countl_zero(ceil) - 1;
}
