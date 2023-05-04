#include "./copy.hpp"

void bits::memcpy_endian(void *dest, Order destOrder, quint16 destLen,
                         const void *src, Order srcOrder, quint16 srcLen) {
  // At most 1 offset will be used at a time, determined by which pointer is
  // longer.
  quint16 srcOffset = 0, destOffset = 0;
  // If src is big endian, we need any 0's to appear on left.
  // src must be shifted when it is longer than dest for 0's to remain.
  if (srcLen > destLen && srcOrder == Order::BigEndian)
    srcOffset = srcLen - destLen;
  // Ibid with src and dest reversed.
  else if (destLen > srcLen && destOrder == Order::BigEndian)
    destOffset = destLen - srcLen;

  // length now points past end of src, must be adjusted
  quint16 adjustedSrcLen = srcLen - srcOffset;
  const quint8 *adjustedSrc = static_cast<const quint8 *>(src) + srcOffset;

  if (srcOrder == destOrder)
    std::copy(adjustedSrc, adjustedSrc + qMin(adjustedSrcLen, destLen),
              static_cast<quint8 *>(dest) + destOffset);
  else
    std::reverse_copy(adjustedSrc, adjustedSrc + qMin(adjustedSrcLen, destLen),
                      static_cast<quint8 *>(dest) + destOffset);
}

// TODO: might be able to vectorize this for large lens.
void bits::memcpy_xor(void *dest, const void *src1, const void *src2,
                      quint16 len) {
  for (auto it = 0; it < len; it++) {
    *static_cast<quint8 *>(dest) =
        *static_cast<const quint8 *>(src1) ^ *static_cast<const quint8 *>(src2);
  }
}
