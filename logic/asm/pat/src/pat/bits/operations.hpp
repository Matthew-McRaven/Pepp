#pragma once
#include "./order.hpp"
#include <QtCore>
namespace pat::bits {
bool copy(const quint8 *src, BitOrder srcOrder, quint16 srcLength, quint8 *dest,
          BitOrder destOrder, quint16 destLength);
}
