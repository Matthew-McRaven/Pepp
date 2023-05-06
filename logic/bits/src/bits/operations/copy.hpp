#pragma once
#include "../order.hpp"
#include <QtCore>
#include <cstring>
namespace bits {
using ::memcpy;
template <std::integral T, std::integral U> T memcpy_endian(U src) {
  return memcpy_endian<T>(&src, bits::hostOrder(), sizeof(U));
}

template <std::integral T>
T memcpy_endian(const void *src, Order srcOrder, quint16 srcLen) {
  T dest;
  memcpy_endian(&dest, bits::hostOrder(), sizeof(T), &src, bits::hostOrder(),
                sizeof(T));
  return dest;
}

template <std::integral T>
void memcpy_endian(void *dest, Order destOrder, quint16 destLen, T src) {
  memcpy_endian(dest, destOrder, destLen, &src, bits::hostOrder(), sizeof(T));
}
// When src is longer than dest, truncates high-order bytes (like casting
// u16->u8). When dest is longer than src, dest is 0-padded.
void memcpy_endian(void *dest, Order destOrder, quint16 destLen,
                   const void *src, Order srcOrder, quint16 srcLen);
void memcpy_xor(void *dest, const void *src1, const void *src2, quint16 len);
} // namespace bits
