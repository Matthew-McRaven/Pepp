#pragma once
#include "../order.hpp"
#include "bits/span.hpp"
#include <QtCore>
#include <cstring>

namespace bits {
using ::memcpy;
void memclr(void *dest, quint16 length);
template <std::integral T, std::integral U> T memcpy_endian(U src) {
  return memcpy_endian<T>({&src, sizeof(U)}, bits::hostOrder());
}

// When src is longer than dest, truncates high-order bytes (like casting
// u16->u8). When dest is longer than src, dest is 0-padded.
void memcpy_endian(span<quint8> dest, Order destOrder, span<const quint8> src,
                   Order srcOrder);

template <std::integral T>
T memcpy_endian(span<const quint8> src, Order srcOrder) {
  T dest = 0;
  memcpy_endian(span<quint8>{reinterpret_cast<quint8 *>(&dest), sizeof(T)},
                bits::hostOrder(), src, srcOrder);
  return dest;
}

template <std::integral T>
void memcpy_endian(span<quint8> dest, Order destOrder, T src) {
  memcpy_endian(dest, destOrder,
                span{reinterpret_cast<const quint8 *>(&src), sizeof(T)},
                bits::hostOrder());
}

void memcpy_xor(bits::span<quint8> dest, bits::span<const quint8> src1,
                bits::span<const quint8> src2);
} // namespace bits
