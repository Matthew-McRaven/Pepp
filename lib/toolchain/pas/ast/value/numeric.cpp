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

#include "numeric.hpp"
#include "core/bitmanip/copy.hpp"
pas::ast::value::Numeric::Numeric() : Base() {}

pas::ast::value::Numeric::Numeric(qint64 value, quint8 size) : _size(size), _value(value) {
  if (size > 8) {
    static const char *const e = "Numeric constants must be <=8 bytes";
    qCritical(e);
    throw std::logic_error(e);
  }
}

void pas::ast::value::Numeric::value(bits::span<quint8> dest, bits::Order destEndian) const {
  using size_type = bits::span<const quint8>::size_type;
  bits::memcpy_endian(
      dest, destEndian,
      bits::span<const quint8>{reinterpret_cast<const quint8 *>(&_value), static_cast<size_type>(size())},
      bits::hostOrder());
}

quint64 pas::ast::value::Numeric::size() const { return _size; }

bool pas::ast::value::Numeric::resize(quint64 size) {
  _size = size;
  return true;
}

quint64 pas::ast::value::Numeric::requiredBytes() const { return ceil(log2(_value + 1) / 8); }

pas::ast::value::Numeric::Numeric(const Numeric &other) : Base(), _size(other._size), _value(other._value) {}

pas::ast::value::Numeric &pas::ast::value::Numeric::operator=(const Numeric &other) {
  // Base::operator=(other); // Needed if we add data to Base.
  this->_size = other._size;
  this->_value = other._value;
  return *this;
}
