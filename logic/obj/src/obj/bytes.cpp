#include "./bytes.hpp"
#include "bits/strings.hpp"

struct Buffer {
  quint64 srcLength = 0, dstLength;
  const void *src;
};

QList<quint8> obj::segmentAsAsciiHex(const ELFIO::segment *segment) {
  static const QList<bits::SeparatorRule> rules = {
      {.skipFirst = false, .separator = ' ', .modulus = 1}};
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
  // 2 characters for each byte, 1 byte for each space. Do not leave space for
  // trailing separator.
  QList<quint8> ret(std::max<qsizetype>(0, 3 * rawBytes - 1));
  // Copy over segment's file bytes
  for (auto buffer : buffered) {
    if (it + 1 > ret.size())
      throw std::logic_error("Dest buffer too small");
    auto i = bits::bytesToAsciiHex((char *)ret.data() + it, ret.length() - it,
                                   reinterpret_cast<const quint8 *>(buffer.src),
                                   buffer.srcLength, rules);
    it += i;
  }
  // Copy over 0's in excess of file size, but required for memsize.
  while (it + 1 < ret.size()) {
    auto i = bits::bytesToAsciiHex((char *)ret.data() + it, ret.length() - it,
                                   zero, 1, rules);
    it += i;
  }
  return ret;
}
