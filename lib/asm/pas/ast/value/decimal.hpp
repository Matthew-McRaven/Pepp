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
#include "./numeric.hpp"

namespace pas::ast::value {
class SignedDecimal : public Numeric {
public:
  explicit SignedDecimal();
  SignedDecimal(qint64 value, quint16 size);
  SignedDecimal(const SignedDecimal &other);
  SignedDecimal(SignedDecimal &&other) noexcept;
  SignedDecimal &operator=(SignedDecimal other);
  friend void swap(SignedDecimal &first, SignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }
  bool isSigned() const override { return true; }
  QSharedPointer<Base> clone() const override;
  // Must negate value before computing max bit value
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;
};

class UnsignedDecimal : public Numeric {
public:
  explicit UnsignedDecimal();
  UnsignedDecimal(quint64 value, quint16 size);
  UnsignedDecimal(const UnsignedDecimal &other);
  UnsignedDecimal(UnsignedDecimal &&other) noexcept;
  UnsignedDecimal &operator=(UnsignedDecimal other);
  friend void swap(UnsignedDecimal &first, UnsignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
  QString rawString() const override;
};
} // namespace pas::ast::value
