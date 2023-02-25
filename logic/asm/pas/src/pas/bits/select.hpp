#pragma once
#include <QtCore>
namespace pas::bits {
struct BitSelection {
  qsizetype byteOffset;
  quint8 bitmask;
  enum class Operation {
    kAND,
    kOR,
    kXOR,
  };
  Operation op;
};
struct ByteSelection {
  qsizetype srcByteOffset, destByteOffset;
  quint16 count;
};
void bitSelect(const quint8 src, BitSelection srcOp, quint8 &out,
               BitSelection destOp);
} // namespace pas::bits
