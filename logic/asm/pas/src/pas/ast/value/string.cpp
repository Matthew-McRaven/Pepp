#include "./string.hpp"
#include "bits/operations/copy.hpp"
#include "bits/strings.hpp"
pas::ast::value::ShortString::ShortString() : Base() {}

pas::ast::value::ShortString::ShortString(QString value, quint8 size,
                                          bits::Order endian)
    : _size(size), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay)
    throw std::logic_error("Invalid escape sequence in string");
  else if (_valueAsBytes.length() > 2)
    throw std::logic_error("Too many bytes for short string");
}

pas::ast::value::ShortString::ShortString(const ShortString &other)
    : Base(), _size(other._size), _value(other._value),
      _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::ShortString::ShortString(ShortString &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::ShortString &
pas::ast::value::ShortString::operator=(ShortString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::ShortString::clone() const {
  return QSharedPointer<ShortString>::create(*this);
}

void pas::ast::value::ShortString::value(bits::span<quint8> dest,
                                         bits::Order destEndian) const {
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{
          reinterpret_cast<const quint8 *>(_valueAsBytes.data()), size()},
      bits::hostOrder());
}

quint64 pas::ast::value::ShortString::size() const { return _size; }

quint64 pas::ast::value::ShortString::requiredBytes() const { return _size; }

QString pas::ast::value::ShortString::string() const {
  return u"\"%1\""_qs.arg(_value);
}

QString pas::ast::value::ShortString::rawString() const { return _value; }

pas::ast::value::LongString::LongString() : Base() {}

pas::ast::value::LongString::LongString(QString value, bits::Order endian)
    : _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay)
    throw std::logic_error("Invalid escape sequence in string");
}

pas::ast::value::LongString::LongString(const LongString &other)
    : Base(), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::LongString::LongString(LongString &&other) noexcept {
  swap(*this, other);
}

pas::ast::value::LongString &
pas::ast::value::LongString::operator=(LongString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base>
pas::ast::value::LongString::clone() const {
  return QSharedPointer<LongString>::create(*this);
}

void pas::ast::value::LongString::value(bits::span<quint8> dest,
                                        bits::Order destEndian) const {
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{
          reinterpret_cast<const quint8 *>(_valueAsBytes.data()), size()},
      bits::hostOrder());
}

quint64 pas::ast::value::LongString::size() const {
  return _valueAsBytes.size();
}

quint64 pas::ast::value::LongString::requiredBytes() const {
  return _valueAsBytes.size();
}

QString pas::ast::value::LongString::string() const {
  return u"\"%1\""_qs.arg(_value);
}

QString pas::ast::value::LongString::rawString() const { return _value; }
