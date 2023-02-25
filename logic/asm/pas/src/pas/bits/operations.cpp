#include "./operations.hpp"

bool pas::bits::copy(const quint8 *src, BitOrder srcOrder, quint16 srcLength,
                     quint8 *dest, BitOrder destOrder, quint16 destLength) {
  // BUG: reversed probably wrong when sizes are different. Zero padding will be
  // added in wrong spot.
  if (srcOrder == destOrder)
    std::copy(src, src + qMin(srcLength, destLength), dest);
  else
    std::reverse_copy(src, src + qMin(srcLength, destLength), dest);
  return true;
}
