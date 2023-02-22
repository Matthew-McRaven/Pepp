#include "decimal.hpp"

pat::ast::argument::SignedDecimal::SignedDecimal() : Numeric() {}

pat::ast::argument::SignedDecimal::SignedDecimal(qint64 value, quint16 size,
                                                 bits::BitOrder endian)
    : Numeric(value, size, endian) {}

pat::ast::argument::SignedDecimal::SignedDecimal(const SignedDecimal &other)
    : Numeric(other) {}

pat::ast::argument::SignedDecimal::SignedDecimal(
    SignedDecimal &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::SignedDecimal &
pat::ast::argument::SignedDecimal::operator=(SignedDecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::argument::Base>
pat::ast::argument::SignedDecimal::clone() const {
  return QSharedPointer<SignedDecimal>::create(*this);
}

QString pat::ast::argument::SignedDecimal::string() const {
  throw std::logic_error("Unimplemented");
}

pat::ast::argument::UnsignedDecimal::UnsignedDecimal() : Numeric() {}

pat::ast::argument::UnsignedDecimal::UnsignedDecimal(quint64 value,
                                                     quint16 size,
                                                     bits::BitOrder endian)
    : Numeric(value, size, endian) {}

pat::ast::argument::UnsignedDecimal::UnsignedDecimal(
    const UnsignedDecimal &other)
    : Numeric(other) {}

pat::ast::argument::UnsignedDecimal::UnsignedDecimal(
    UnsignedDecimal &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::UnsignedDecimal &
pat::ast::argument::UnsignedDecimal::operator=(UnsignedDecimal other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::argument::Base>
pat::ast::argument::UnsignedDecimal::clone() const {
  return QSharedPointer<UnsignedDecimal>::create(*this);
}

QString pat::ast::argument::UnsignedDecimal::string() const {
  throw std::logic_error("Unimplemented");
}
