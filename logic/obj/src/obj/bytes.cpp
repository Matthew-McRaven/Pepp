#include "./bytes.hpp"
#include "bits/strings.hpp"

struct Buffer {
  quint64 srcLength = 0, dstLength;
  const void *src;
};

QList<quint8> obj::segmentAsAsciiHex(const ELFIO::segment *segment) {
  QList<Buffer> buffered;
  qsizetype rawBytes = 0;
  if (segment->get_type() != ELFIO::PT_LOPROC + 1)
    return {};
  rawBytes += segment->get_memory_size();
  buffered.push_back(Buffer{.srcLength = segment->get_file_size(),
                            .dstLength = segment->get_memory_size(),
                            .src = segment->get_data()});
  static const quint8 zero[] = {0x00};
  qsizetype it = 0;
  // 2 characters for each byte, 1 byte for each space
  QList<quint8> ret(std::max<qsizetype>(0, 3 * rawBytes - 1));
  ret.last() = 0;
  for (auto buffer : buffered) {
    if (it + 1 > ret.size())
      throw std::logic_error("Dest buffer too small");
    auto i = bits::bytesToAsciiHex((char *)ret.data() + it, ret.length() - it,
                                   reinterpret_cast<const quint8 *>(buffer.src),
                                   buffer.srcLength, true);
    it += i;
  }
  while (it + 1 < ret.size()) {
    auto i = bits::bytesToAsciiHex((char *)ret.data() + it, ret.length() - it,
                                   zero, 1, false);
    it += i;
  }
  return ret;
}
