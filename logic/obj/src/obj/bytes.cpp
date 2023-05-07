#include "./bytes.hpp"
#include "bits/strings.hpp"

struct Buffer {
  quint64 length = 0;
  const void *data;
};

QList<quint8> obj::bufferedSegmentsAsAsciiHex(const ELFIO::elfio &elf,
                                              QString suffix) {
  QList<Buffer> buffered;
  qsizetype rawBytes = 0;
  for (auto &segment : elf.segments) {
    if (segment->get_type() != ELFIO::PT_LOPROC + 1)
      continue;
    rawBytes += segment->get_file_size();
    buffered.push_back(Buffer{.length = segment->get_file_size(),
                              .data = segment->get_data()});
  }
  const auto asStd = suffix.toStdString();
  buffered.push_back({.length = asStd.length(), .data = asStd.data()});
  qsizetype it = 0;
  // 2 characters for each byte, 1 byte for each space
  QList<quint8> ret(3 * rawBytes + suffix.length());
  for (auto buffer : buffered) {
    if (it > ret.size())
      throw std::logic_error("Dest buffer too small");
    auto i = bits::bytesToAsciiHex(
        ret.data() + it, ret.length() - it,
        reinterpret_cast<const quint8 *>(buffer.data), buffer.length);
    if (i < 3 * buffer.length)
      throw std::logic_error("Failed to copy all bytes to dest buffer");
    it += i;
  }
  return ret;
}
