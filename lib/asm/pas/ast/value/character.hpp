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
class Character : public Base {
public:
  explicit Character();
  explicit Character(QString value);
  Character(const Character &other);
  Character(Character &&other) noexcept;
  Character &operator=(Character other);
  friend void swap(Character &first, Character &second) {
    using std::swap;
    swap(first._value, second._value);
    swap(first._valueAsBytes, second._valueAsBytes);
  }

  bool isNumeric() const override { return true; }
  bool isFixedSize() const override { return true; }
  bool isWide() const override { return false; }
  bool isText() const override { return true; }
  bool isIdentifier() const override { return false; }
  bool isSigned() const override { return false; }
  QSharedPointer<Base> clone() const override;
  void value(bits::span<quint8> dest, bits::Order destEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  bool resize(quint64 size) override;
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;

private:
  QString _value = {};
  QByteArray _valueAsBytes = {};
};
}; // namespace pas::ast::value
