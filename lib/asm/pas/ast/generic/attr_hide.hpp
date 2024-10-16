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

namespace pas::ast::generic {
struct Hide {
  static const inline QString attributeName = "generic:hide";
  static const inline uint8_t attribute = 10;
  struct In {
    bool source = false;
    bool listing = false;
    enum class Object { NoEmit_CountSize, NoEmit_NoCountSize, Emit } object = Object::Emit;
    bool addressInListing = false;

    bool operator==(const In &other) const = default;
  } value = {};
  bool operator==(const Hide &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::Hide);
