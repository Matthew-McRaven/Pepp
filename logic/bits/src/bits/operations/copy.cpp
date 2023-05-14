#include "./copy.hpp"

void bits::memclr(void *dest, quint16 length) { memset(dest, 0, length); }

void bits::memcpy_endian(std::span<quint8> dest, Order destOrder,
                         std::span<const quint8> src, Order srcOrder) {
  // At most 1 offset will be used at a time, determined by which pointer is
  // longer.
  std::size_t srcOffset = 0, destOffset = 0;
  // If src is big endian, we need any 0's to appear on left.
  // src must be shifted when it is longer than dest for 0's to remain.
  if (src.size() > dest.size() && srcOrder == Order::BigEndian)
    srcOffset = src.size() - dest.size();
  // Ibid with src and dest reversed.
  else if (dest.size() > src.size() && destOrder == Order::BigEndian)
    destOffset = dest.size() - src.size();

  // length now points past end of src, must be adjusted
  auto adjustedDest = dest.subspan(destOffset);
  // Src is iterated from start to end, so it must contain no more than
  // adjustedDest elements.
  auto adjustedSrc =
      src.subspan(srcOffset, std::min(src.size() - srcOffset, dest.size()));

  if (srcOrder == destOrder)
    std::copy(adjustedSrc.begin(), adjustedSrc.end(), adjustedDest.begin());
  else
    std::reverse_copy(adjustedSrc.begin(), adjustedSrc.end(),
                      adjustedDest.begin());
}

// TODO: might be able to vectorize this for large lens.
void bits::memcpy_xor(void *dest, const void *src1, const void *src2,
                      quint16 len) {
  for (auto it = 0; it < len; it++) {
    *(static_cast<quint8 *>(dest) + it) =
        *(static_cast<const quint8 *>(src1) + it) ^
        *(static_cast<const quint8 *>(src2) + it);
  }
}
