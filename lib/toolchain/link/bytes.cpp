/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "./bytes.hpp"
#include "core/libs/bitmanip/strings.hpp"

struct Buffer {
  quint64 srcLength = 0, dstLength;
  const void *src;
};

QList<quint8> obj::segmentAsAsciiHex(const ELFIO::segment *segment) {
  using size_type=std::span<const quint8>::size_type;
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
  std::size_t it = 0;
  // 2 characters for each byte, 1 byte for each space. Do not leave space for
  // trailing separator.
  QList<quint8> ret(std::max<qsizetype>(0, 3 * rawBytes - 1));
  // Copy over segment's file bytes
  for (auto buffer : buffered) {
    if (it + 1 > ret.size()) {
      static const char *const e = "Dest buffer too small";
      qCritical(e);
      throw std::logic_error(e);
    }
    auto i = bits::bytesToAsciiHex(
        {(char *)ret.data() + it, ret.length() - it},
        {reinterpret_cast<const quint8 *>(buffer.src),
         static_cast<size_type>(buffer.srcLength)},
        rules);
    it += i;
  }
  // Copy over 0's in excess of file size, but required for memsize.
  while (it + 1 < ret.size()) {
    auto i = bits::bytesToAsciiHex({(char *)ret.data() + it, ret.length() - it},
                                   {zero, 1}, rules);
    it += i;
  }
  return ret;
}
