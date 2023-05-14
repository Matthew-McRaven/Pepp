#include "character.hpp"
#include "bits/operations/copy.hpp"
#include "bits/strings.hpp"
pas::ast::value::Character::Character() : Base() {}

pas::ast::value::Character::Character(QString value)
    : Base(), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay)
    throw std::logic_error("Invalid escape sequence in character");
  else if (_valueAsBytes.length() > 1)
    throw std::logic_error("Too many bytes for character");
}

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

void pas::ast::value::Character::value(bits::span<quint8> dest,
                                       bits::Order destEndian) const {
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{
          reinterpret_cast<const quint8 *>(_valueAsBytes.constData()), size()},
      bits::hostOrder());
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
