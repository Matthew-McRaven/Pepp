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
#include "asm/pas/pas_globals.hpp"

namespace pas::ast::value {
struct PAS_EXPORT Identifier : public Base {
public:
  explicit Identifier();
  explicit Identifier(QString value);
  Identifier(const Identifier &other);
  Identifier(Identifier &&other) noexcept;
  Identifier &operator=(Identifier other);
  friend void swap(Identifier &first, Identifier &second) {
    using std::swap;
    swap(first._value, second._value);
  }

  bool isNumeric() const override { return false; }
  bool isFixedSize() const override { return false; }
  bool isWide() const override { return false; }
  bool isText() const override { return false; }
  bool isIdentifier() const override { return true; }
  bool isSigned() const override { return false; }
  QSharedPointer<Base> clone() const override;
  void value(bits::span<quint8> dest,
             bits::Order destEndian = bits::hostOrder()) const override;
  quint64 size() const override;
  quint64 requiredBytes() const override;
  QString string() const override;
  QString rawString() const override;

private:
  QString _value = {};
};
} // namespace pas::ast::value
