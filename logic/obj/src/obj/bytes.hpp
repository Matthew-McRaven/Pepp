#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>

namespace obj {
// WARNING: does not work well with non-contiguous buffered segments.
QList<quint8> bufferedSegmentsAsAsciiHex(const ELFIO::elfio &elf,
                                         QString suffix = "zz");
} // namespace obj
