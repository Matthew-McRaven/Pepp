#include "./string.hpp"
#include "../../bits/operations.hpp"
#include "../../bits/strings.hpp"
pat::ast::argument::ShortString::ShortString() : Base() {}

pat::ast::argument::ShortString::ShortString(QString value, quint8 size,
                                             bits::BitOrder endian)
    : _endian(endian), _size(size), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay)
    throw std::logic_error("Invalid escape sequence in string");
  else if (_valueAsBytes.length() > 2)
    throw std::logic_error("Too many bytes for short string");
}

pat::ast::argument::ShortString::ShortString(const ShortString &other)
    : Base(), _endian(other._endian), _size(other._size), _value(other._value),
      _valueAsBytes(other._valueAsBytes) {}

pat::ast::argument::ShortString::ShortString(ShortString &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::ShortString &
pat::ast::argument::ShortString::operator=(ShortString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::Value> pat::ast::argument::ShortString::clone() const {
  return QSharedPointer<ShortString>::create(*this);
}

pat::bits::BitOrder pat::ast::argument::ShortString::endian() const {
  return _endian;
}

bool pat::ast::argument::ShortString::value(quint8 *dest,
                                            quint16 length) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, _endian, length);
}

quint64 pat::ast::argument::ShortString::size() const { return _size; }

bool pat::ast::argument::ShortString::bits(QByteArray &out,
                                           bits::BitSelection src,
                                           bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::argument::ShortString::bytes(QByteArray &out, qsizetype start,
                                            qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::argument::ShortString::string() const {
  throw std::logic_error("Unimplemented");
}

pat::ast::argument::LongString::LongString() : Base() {}

pat::ast::argument::LongString::LongString(QString value, bits::BitOrder endian)
    : _endian(endian), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay)
    throw std::logic_error("Invalid escape sequence in string");
}

pat::ast::argument::LongString::LongString(const LongString &other)
    : Base(), _endian(other._endian), _value(other._value),
      _valueAsBytes(other._valueAsBytes) {}

pat::ast::argument::LongString::LongString(LongString &&other) noexcept {
  swap(*this, other);
}

pat::ast::argument::LongString &
pat::ast::argument::LongString::operator=(LongString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::Value> pat::ast::argument::LongString::clone() const {
  return QSharedPointer<LongString>::create(*this);
}

pat::bits::BitOrder pat::ast::argument::LongString::endian() const {
  return _endian;
}

bool pat::ast::argument::LongString::value(quint8 *dest, quint16 length) const {
  return bits::copy(reinterpret_cast<const quint8 *>(_valueAsBytes.data()),
                    bits::hostOrder(), size(), dest, _endian, length);
}

quint64 pat::ast::argument::LongString::size() const {
  return _valueAsBytes.size();
}

bool pat::ast::argument::LongString::bits(QByteArray &out,
                                          bits::BitSelection src,
                                          bits::BitSelection dest) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::argument::LongString::bytes(QByteArray &out, qsizetype start,
                                           qsizetype length) const {
  throw std::logic_error("Unimplemented");
}

QString pat::ast::argument::LongString::string() const {
  throw std::logic_error("Unimplemented");
}
