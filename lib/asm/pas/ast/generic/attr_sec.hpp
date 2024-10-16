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

struct SectionFlags {
  static const inline QString attributeName = "generic:section_flags";
  static const inline uint8_t attribute = 14;
  struct Flags {
    bool R = 1, W = 1, X = 1, Z = 0;
    bool operator==(const Flags &other) const = default;
  } value = {};
  bool operator==(const SectionFlags &other) const = default;
};
struct SectionName {
  static const inline QString attributeName = "generic:section_name";
  static const inline uint8_t attribute = 15;
  QString value = {};
  bool operator==(const SectionName &other) const = default;
};

} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::SectionFlags);
Q_DECLARE_METATYPE(pas::ast::generic::SectionName);
