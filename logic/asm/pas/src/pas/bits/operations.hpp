#pragma once
#include "./order.hpp"
#include <QtCore>
namespace pas::bits {

// When src is longer than dest, truncates high-order bytes (like casting
// u16->u8). When dest is longer than src, dest is 0-padded.
bool copy(const quint8 *src, BitOrder srcOrder, quint16 srcLength, quint8 *dest,
          BitOrder destOrder, quint16 destLength);
} // namespace pas::bits
