#include "./order.hpp"

#include <QSysInfo>

pas::bits::BitOrder pas::bits::hostOrder() {
  switch (QSysInfo::ByteOrder) {
  case QSysInfo::LittleEndian:
    return BitOrder::LittleEndian;
  case QSysInfo::BigEndian:
    return BitOrder::BigEndian;
  default:
    throw std::logic_error("Endian must be big or little");
  }
}
