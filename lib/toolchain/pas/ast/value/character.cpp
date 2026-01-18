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

#include "character.hpp"
#include "bts/bitmanip/copy.hpp"
#include "bts/bitmanip/strings.hpp"
#include "toolchain/pas/string_utils.hpp"
pas::ast::value::Character::Character() : Base() {}

pas::ast::value::Character::Character(QString value) : Base(), _value(value), _valueAsBytes({}) {
  bool okay = bits::escapedStringToBytes(value, _valueAsBytes);
  if (!okay) {
    static const char *const e = "Invalid escape sequence in character";
    qCritical(e);
    throw std::logic_error(e);
  } else if (_valueAsBytes.length() > 1) {
    static const char *const e = "Too many bytes for character";
    qCritical(e);
    throw std::logic_error(e);
  }
}

pas::ast::value::Character::Character(const Character &other)
    : Base(), _value(other._value), _valueAsBytes(other._valueAsBytes) {}

pas::ast::value::Character::Character(Character &&other) noexcept : Character() { swap(*this, other); }

pas::ast::value::Character &pas::ast::value::Character::operator=(Character other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pas::ast::value::Base> pas::ast::value::Character::clone() const {
  return QSharedPointer<Character>::create(*this);
}

void pas::ast::value::Character::value(bits::span<quint8> dest, bits::Order destEndian) const {
  using size_type = bits::span<const quint8>::size_type;
  bits::memcpy_endian(dest, destEndian,
                      bits::span<const quint8>{reinterpret_cast<const quint8 *>(_valueAsBytes.constData()),
                                               static_cast<size_type>(size())},
                      bits::hostOrder());
}

quint64 pas::ast::value::Character::size() const { return _valueAsBytes.size(); }

bool pas::ast::value::Character::resize(quint64 size) {
  _valueAsBytes.resize(size, 0);
  return true;
}

quint64 pas::ast::value::Character::requiredBytes() const { return _valueAsBytes.size(); }

QString pas::ast::value::Character::string() const {
  using namespace Qt::StringLiterals;
  return u"'%1'"_s.arg(_value);
}

QString pas::ast::value::Character::rawString() const { return _value; }
