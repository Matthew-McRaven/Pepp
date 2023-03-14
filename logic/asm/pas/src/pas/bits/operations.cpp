#include "./operations.hpp"

bool pas::bits::copy(const quint8 *src, BitOrder srcOrder, quint16 srcLength,
                     quint8 *dest, BitOrder destOrder, quint16 destLength) {
  // At most 1 offset will be used at a time, determined by which pointer is
  // longer.
  quint16 srcOffset = 0, destOffset = 0;
  // If src is big endian, we need any 0's to appear on left.
  // src must be shifted when it is longer than dest for 0's to remain.
  if (srcLength > destLength && srcOrder == BitOrder::BigEndian)
    srcOffset = srcLength - destLength;
  // Ibid with src and dest reversed.
  else if (destLength > srcLength && destOrder == BitOrder::BigEndian)
    destOffset = destLength - srcLength;

  // length now points past end of src, must be adjusted
  quint16 adjustedSrcLen = srcLength - srcOffset;
  const quint8 *adjustedSrc = src + srcOffset;

  if (srcOrder == destOrder)
    std::copy(adjustedSrc, adjustedSrc + qMin(adjustedSrcLen, destLength),
              dest + destOffset);
  else
    std::reverse_copy(adjustedSrc,
                      adjustedSrc + qMin(adjustedSrcLen, destLength),
                      dest + destOffset);
  return true;
}
