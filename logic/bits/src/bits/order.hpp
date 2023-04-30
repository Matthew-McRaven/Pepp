#pragma once
#include <QSysInfo>
#include <QtCore>
namespace bits {
enum class Order {
  BigEndian = QSysInfo::Endian::BigEndian,
  LittleEndian = QSysInfo::Endian::LittleEndian,
  NotApplicable
};
constexpr Order hostOrder() {
  switch (QSysInfo::ByteOrder) {
  case QSysInfo::LittleEndian:
    return Order::LittleEndian;
  case QSysInfo::BigEndian:
    return Order::BigEndian;
  default:
    throw std::logic_error("Endian must be big or little");
  }
}

} // namespace bits
