#pragma once
#include <QtCore>
namespace pas::bits {
enum class BitOrder { BigEndian, LittleEndian, NotApplicable };
BitOrder hostOrder();

} // namespace pas::bits
