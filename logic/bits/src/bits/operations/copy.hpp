#pragma once
#include "../order.hpp"
#include <QtCore>
#include <cstring>
namespace bits {
using ::memcpy;
// When src is longer than dest, truncates high-order bytes (like casting
// u16->u8). When dest is longer than src, dest is 0-padded.
void memcpy_endian(void *dest, Order destOrder, quint16 destLen,
                   const void *src, Order srcOrder, quint16 srcLen);
void memcpy_xor(void *dest, const void *src1, const void *src2, quint16 len);
} // namespace bits
