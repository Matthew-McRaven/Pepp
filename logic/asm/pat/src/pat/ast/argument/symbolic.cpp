#include "./symbolic.hpp"
#include "../../bits/operations.hpp"
#include "../../bits/order.hpp"
#include "symbol/entry.hpp"
pat::ast::argument::Symbolic::Symbolic() {}

pat::ast::argument::Symbolic::Symbolic(QSharedPointer<symbol::Entry> value,
                                       bits::BitOrder endian)
    : Base(), _value(value) {}

pat::ast::argument::Symbolic::Symbolic(const Symbolic &other) : Base() {}

pat::ast::argument::Symbolic::Symbolic(Symbolic &&other) noexcept : Symbolic() {
  swap(*this, other);
}

pat::ast::argument::Symbolic &
pat::ast::argument::Symbolic::operator=(Symbolic other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::argument::Base>
pat::ast::argument::Symbolic::clone() const {
  // BUG: Symbol needs to be forked into a new table!!
  return QSharedPointer<Symbolic>::create(*this);
}

bool pat::ast::argument::Symbolic::value(quint8 *dest, qsizetype length,
                                         bits::BitOrder destEndian) const {
  auto src = _value->value->value()();
  return bits::copy(reinterpret_cast<quint8 *>(&src), bits::hostOrder(), size(),
                    dest, destEndian, length);
}

quint64 pat::ast::argument::Symbolic::size() const {
  return _value->value->value().byteCount;
}

QString pat::ast::argument::Symbolic::string() const { return _value->name; }
