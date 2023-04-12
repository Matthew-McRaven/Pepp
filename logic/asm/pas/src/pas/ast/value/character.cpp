#include "character.hpp"
#include "../../bits/operations.hpp"
pas::ast::value::Character::Character() : Base() {}

pas::ast::value::Character::Character(QString value)
    : Base(), _value(value), _valueAsBytes({}) {}

pas::ast::value::Character::Character(const Character &other)
    : Base(), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::Character::Character(Character &&other) noexcept
    : Character() {
  swap(*this, other);
}

pas::ast::value::Character &
pas::ast::value::Character::operator=(Character other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::Character::clone() const {
  return QSharedPointer<Character>::create(*this);
}

bool pas::ast::value::Character::value(quint8 *dest, qsizetype length,
                                       bits::BitOrder destEndian) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, destEndian, length);
}

quint64 pas::ast::value::Character::size() const {
  return _valueAsBytes.size();
}

quint64 pas::ast::value::Character::requiredBytes() const {
  return _valueAsBytes.size();
}

QString pas::ast::value::Character::string() const {
  return u"'%1'"_qs.arg(_value);
}

QString pas::ast::value::Character::rawString() const { return _value; }
