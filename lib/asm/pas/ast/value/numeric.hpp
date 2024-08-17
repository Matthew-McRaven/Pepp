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

#pragma once
#include "./base.hpp"

namespace pas::ast::value {
struct Numeric : public Base {
public:
  explicit Numeric();
  Numeric(qint64 value, quint8 size);
  friend void swap(Numeric &first, Numeric &second) {
    using std::swap;
    swap((Base &)first, (Base &)second);
    swap(first._size, second._size);
    swap(first._value, second._value);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return false; }
  bool isSigned() const override { return false; }
  virtual QSharedPointer<Base> clone() const override = 0;
  void value(bits::span<quint8> dest, bits::Order targetEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  bool resize(quint64 size) override;
  quint64 requiredBytes() const override;
  virtual QString string() const override = 0;

protected:
  Numeric(const Numeric &other);
  Numeric &operator=(const Numeric &other);
  quint8 _size = 0;
  qint64 _value = 0;
};
} // namespace pas::ast::value
