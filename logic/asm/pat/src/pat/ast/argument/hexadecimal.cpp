#include "hexadecimal.hpp"

pat::ast::argument::Hexadecimal::Hexadecimal(quint64 value, quint16 size,
                                             bits::BitOrder endian)
    : Numeric(value, size, endian) {}

QSharedPointer<pat::ast::Value> pat::ast::argument::Hexadecimal::clone() const {
  return QSharedPointer<Hexadecimal>::create(_value, _size, _endian);
}

QString pat::ast::argument::Hexadecimal::string() const {
  throw std::logic_error("Unimplemented");
}
