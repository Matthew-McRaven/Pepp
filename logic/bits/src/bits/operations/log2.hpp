#pragma once
#include <QtCore>

namespace bits {
/*
 * 01 => 0
 * 02 => 1
 * 04 => 2
 * 08 => 3
 * 16 => 4
 * 32 => 5
 * etc
 *
 *Will throw if value == 0.
 */
quint8 ceil_log2(quint64 value);
} // namespace bits
