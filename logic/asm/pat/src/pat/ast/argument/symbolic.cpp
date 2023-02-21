#include "./symbolic.hpp"
#include "../../bits/operations.hpp"
#include "../../bits/order.hpp"
#include "symbol/entry.hpp"
pat::ast::argument::Symbolic::Symbolic() {}

pat::ast::argument::Symbolic::Symbolic(QSharedPointer<symbol::Entry> value,
                                       bits::BitOrder endian)
    : Base(), _endian(endian), _value(value) {}

pat::ast::argument::Symbolic::Symbolic(const Symbolic &other)
    : Base(), _endian(other._endian), _value(other._value) {}

pat::ast::argument::Symbolic::Symbolic(Symbolic &&other) noexcept : Symbolic() {
  swap(*this, other);
}

pat::ast::argument::Symbolic &
pat::ast::argument::Symbolic::operator=(Symbolic other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::Value> pat::ast::argument::Symbolic::clone() const {
  // BUG: Symbol needs to be forked into a new table!!
  return QSharedPointer<Symbolic>::create(*this);
}

pat::bits::BitOrder pat::ast::argument::Symbolic::endian() const {
  return _endian;
}

bool pat::ast::argument::Symbolic::value(quint8 *dest, quint16 length) const {
  auto src = _value->value->value()();
  return bits::copy(reinterpret_cast<quint8 *>(&src), bits::hostOrder(), size(),
                    dest, _endian, length);
}

quint64 pat::ast::argument::Symbolic::size() const {
  return _value->value->value().byteCount;
}

bool pat::ast::argument::Symbolic::bits(QByteArray &out, bits::BitSelection src,
                                        bits::BitSelection dest) const {
  // BUG: handle endianness
  throw std::logic_error("Unimplemented");
}

bool pat::ast::argument::Symbolic::bytes(QByteArray &out, qsizetype start,
                                         qsizetype length) const {
  // BUG: handle endianness
  throw std::logic_error("Unimplemented");
}

QString pat::ast::argument::Symbolic::string() const { return _value->name; }
