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
#include <QtCore>

namespace pas::ast::pepp {
template <typename ISA> struct AddressingMode {
  static const inline QString attributeName = "pepp:addr";
  static const inline uint8_t attribute = 1;
  typename ISA::AddressingMode value = {};
  bool operator==(const AddressingMode<ISA> &other) const = default;
};
} // namespace pas::ast::pepp
// Must add this to ISA declaration.
// Q_DECLARE_METATYPE(pas::ast::pepp::AddressingMode<T>);
