#include "decimal.hpp"

pat::ast::argument::SignedDecimal::SignedDecimal(qint64 value, quint16 size,
                                                 bits::BitOrder endian)
    : Numeric(value, size, endian) {}

QSharedPointer<pat::ast::Value>
pat::ast::argument::SignedDecimal::clone() const {
  return QSharedPointer<SignedDecimal>::create(_value, _size, _endian);
}

QString pat::ast::argument::SignedDecimal::string() const {
  throw std::logic_error("Unimplemented");
}

pat::ast::argument::UnsignedDecimal::UnsignedDecimal(quint64 value,
                                                     quint16 size,
                                                     bits::BitOrder endian)
    : Numeric(value, size, endian) {}

QSharedPointer<pat::ast::Value>
pat::ast::argument::UnsignedDecimal::clone() const {
  return QSharedPointer<UnsignedDecimal>::create(_value, _size, _endian);
}

QString pat::ast::argument::UnsignedDecimal::string() const {
  throw std::logic_error("Unimplemented");
}
