#include "mask.hpp"

uint64_t bits::mask(uint8_t byteCount) {
  if (byteCount >= 8) return -1;
  return (1ULL << (byteCount * 8ULL)) - 1ULL;
}
