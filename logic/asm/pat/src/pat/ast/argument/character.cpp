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

QSharedPointer<pat::ast::argument::Base>
pat::ast::argument::Character::clone() const {
  return QSharedPointer<Character>::create(*this);
}

bool pat::ast::argument::Character::value(quint8 *dest, qsizetype length,
                                          bits::BitOrder destEndian) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, destEndian, length);
}

quint64 pat::ast::argument::Character::size() const { return _value.size(); }

QString pat::ast::argument::Character::string() const {
  throw std::logic_error("Unimplemented");
}
