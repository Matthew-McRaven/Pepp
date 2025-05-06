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
struct ShortString : public Base {
public:
  explicit ShortString();
  ShortString(QString value, quint8 size, bits::Order endian);
  ShortString(const ShortString &other);
  ShortString(ShortString &&other) noexcept;
  ShortString &operator=(ShortString other);
  friend void swap(ShortString &first, ShortString &second) {
    using std::swap;
    swap(first._size, second._size);
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
  quint8 _size = 0;
  QString _value = {};
  QByteArray _valueAsBytes = {};
};

struct LongString : public Base {
public:
  explicit LongString();
  LongString(QString value, bits::Order endian);
  LongString(const LongString &other);
  LongString(LongString &&other) noexcept;
  LongString &operator=(LongString other);
  friend void swap(LongString &first, LongString &second) {
    using std::swap;
    swap(first._value, second._value);
    swap(first._valueAsBytes, second._valueAsBytes);
  }

  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return false; }
  bool isWide() const override { return size() > 8; }
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
} // namespace pas::ast::value
