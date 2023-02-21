#include "hexadecimal.hpp"

pat::ast::argument::Hexadecimal::Hexadecimal() : Numeric() {}

pat::ast::argument::Hexadecimal::Hexadecimal(quint64 value, quint16 size,
                                             bits::BitOrder endian)
    : Numeric(value, size, endian) {}

pat::ast::argument::Hexadecimal::Hexadecimal(const Hexadecimal &other)
    : Numeric(other) {}

pat::ast::argument::Hexadecimal::Hexadecimal(Hexadecimal &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::Hexadecimal &
pat::ast::argument::Hexadecimal::operator=(Hexadecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::Value> pat::ast::argument::Hexadecimal::clone() const {
  return QSharedPointer<Hexadecimal>::create(*this);
}

QString pat::ast::argument::Hexadecimal::string() const {
  throw std::logic_error("Unimplemented");
}
