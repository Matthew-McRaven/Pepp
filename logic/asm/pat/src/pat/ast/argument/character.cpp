#include "character.hpp"
#include "../../bits/operations.hpp"
pat::ast::argument::Character::Character() : Base() {}

pat::ast::argument::Character::Character(QString value)
    : Base(), _value(value), _valueAsBytes({}) {}

pat::ast::argument::Character::Character(const Character &other)
    : Base(), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pat::ast::argument::Character::Character(Character &&other) noexcept
    : Character() {
  swap(*this, other);
}

pat::ast::argument::Character &
pat::ast::argument::Character::operator=(Character other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::Value> pat::ast::argument::Character::clone() const {
  return QSharedPointer<Character>::create(*this);
}

pat::bits::BitOrder pat::ast::argument::Character::endian() const {
  return bits::BitOrder::NotApplicable;
}

bool pat::ast::argument::Character::value(quint8 *dest, quint16 length) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, bits::hostOrder(), length);
}

quint64 pat::ast::argument::Character::size() const { return _value.size(); }

bool pat::ast::argument::Character::bits(QByteArray &out,
                                         bits::BitSelection src,
                                         bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::argument::Character::bytes(QByteArray &out, qsizetype start,
                                          qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::argument::Character::string() const {
  throw std::logic_error("Unimplemented");
}
