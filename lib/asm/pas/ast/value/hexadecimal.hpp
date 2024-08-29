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
class Hexadecimal : public Numeric {
public:
  explicit Hexadecimal();
  Hexadecimal(quint64 value, quint16 size);
  Hexadecimal(const Hexadecimal &other);
  Hexadecimal(Hexadecimal &&other) noexcept;
  Hexadecimal &operator=(Hexadecimal other);
  friend void swap(Hexadecimal &first, Hexadecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  QSharedPointer<Base> clone() const override;
  QString string() const override;
  QString rawString() const override;
};
} // namespace pas::ast::value