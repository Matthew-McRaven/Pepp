#include "./symbolic.hpp"
#include "pas/bits/operations.hpp"
#include "pas/bits/order.hpp"
#include "symbol/entry.hpp"
pas::ast::value::Symbolic::Symbolic() {}

pas::ast::value::Symbolic::Symbolic(QSharedPointer<symbol::Entry> value)
    : Base(), _value(value) {}

pas::ast::value::Symbolic::Symbolic(const Symbolic &other) : Base() {}

pas::ast::value::Symbolic::Symbolic(Symbolic &&other) noexcept : Symbolic() {
  swap(*this, other);
}

pas::ast::value::Symbolic &
pas::ast::value::Symbolic::operator=(Symbolic other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<const symbol::Entry> pas::ast::value::Symbolic::symbol() const {
  return _value;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::Symbolic::clone() const {
  // BUG: Symbol needs to be forked into a new table!!
  return QSharedPointer<Symbolic>::create(*this);
}

bool pas::ast::value::Symbolic::value(quint8 *dest, qsizetype length,
                                      bits::BitOrder destEndian) const {
  auto src = _value->value->value()();
  return bits::copy(reinterpret_cast<quint8 *>(&src), bits::hostOrder(), size(),
                    dest, destEndian, length);
}

quint64 pas::ast::value::Symbolic::size() const {
  return _value->value->value().byteCount;
}

quint64 pas::ast::value::Symbolic::requiredBytes() const {
  return ceil(log2(_value->value->value()() + 1) / 8);
}

QString pas::ast::value::Symbolic::string() const { return _value->name; }
