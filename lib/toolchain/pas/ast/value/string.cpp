/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "./string.hpp"
#include "bts/bitmanip/copy.hpp"
#include "bts/bitmanip/strings.hpp"
#include "toolchain/pas/string_utils.hpp"
pas::ast::value::ShortString::ShortString() : Base() {}

pas::ast::value::ShortString::ShortString(QString value, quint8 size, bits::Order endian)
    : _size(size), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay) {
    static const char *const e = "Invalid escape sequence in string";
    qCritical(e);
    throw std::logic_error(e);
  } else if (_valueAsBytes.length() > 2) {
    static const char *const e = "Too many bytes for short string";
    qCritical(e);
    throw std::logic_error(e);
  }
}

pas::ast::value::ShortString::ShortString(const ShortString &other)
    : Base(), _size(other._size), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::ShortString::ShortString(ShortString &&other) noexcept { swap(*this, other); }

pas::ast::value::ShortString &pas::ast::value::ShortString::operator=(ShortString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::ShortString::clone() const {
  return QSharedPointer<ShortString>::create(*this);
}

void pas::ast::value::ShortString::value(bits::span<quint8> dest, bits::Order destEndian) const {
  using size_type = bits::span<const quint8>::size_type;
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{reinterpret_cast<const quint8 *>(_valueAsBytes.data()), static_cast<size_type>(size())},
      bits::hostOrder());
}

quint64 pas::ast::value::ShortString::size() const { return _size; }

bool pas::ast::value::ShortString::resize(quint64 size) { return false; }

quint64 pas::ast::value::ShortString::requiredBytes() const { return _size; }

QString pas::ast::value::ShortString::string() const {
  using namespace Qt::StringLiterals;
  return u"\"%1\""_s.arg(_value);
}

QString pas::ast::value::ShortString::rawString() const { return _value; }

pas::ast::value::LongString::LongString() : Base() {}

pas::ast::value::LongString::LongString(QString value, bits::Order endian) : _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay) {
    static const char *const e = "Invalid escape sequence in string";
    qCritical(e);
    throw std::logic_error(e);
  }
}

pas::ast::value::LongString::LongString(const LongString &other)
    : Base(), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::LongString::LongString(LongString &&other) noexcept { swap(*this, other); }

pas::ast::value::LongString &pas::ast::value::LongString::operator=(LongString other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::LongString::clone() const {
  return QSharedPointer<LongString>::create(*this);
}

void pas::ast::value::LongString::value(bits::span<quint8> dest, bits::Order destEndian) const {
  using size_type = bits::span<const quint8>::size_type;
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{reinterpret_cast<const quint8 *>(_valueAsBytes.data()), static_cast<size_type>(size())},
      bits::hostOrder());
}

quint64 pas::ast::value::LongString::size() const { return _valueAsBytes.size(); }

bool pas::ast::value::LongString::resize(quint64 size) { return false; }

quint64 pas::ast::value::LongString::requiredBytes() const { return _valueAsBytes.size(); }

QString pas::ast::value::LongString::string() const {
  using namespace Qt::StringLiterals;
  return u"\"%1\""_s.arg(_value);
}

QString pas::ast::value::LongString::rawString() const { return _value; }
