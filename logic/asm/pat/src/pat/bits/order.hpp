#pragma once
#include <QtCore>
namespace pat::bits {
enum class BitOrder { BigEndian, LittleEndian, NotApplicable };
BitOrder hostOrder();

} // namespace pat::bits
