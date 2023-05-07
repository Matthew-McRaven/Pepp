#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>

namespace obj {
// WARNING: does not work well with non-contiguous buffered segments.
// Does not emit trailing space.
QList<quint8> segmentAsAsciiHex(const ELFIO::segment *segment);
} // namespace obj
