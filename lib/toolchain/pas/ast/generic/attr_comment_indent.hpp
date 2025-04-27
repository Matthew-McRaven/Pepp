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
struct CommentIndent {
  enum class Level { Left, Instruction };
  static const inline QString attributeName = "generic:comment_indent";
  static const inline uint8_t attribute = 7;
  Level value = Level::Left;
  bool operator==(const CommentIndent &other) const = default;
};
struct IsMacroComment {
  static const inline QString attributeName = "generic:comment_macro";
  static const inline uint8_t attribute = 25;
  bool operator==(const IsMacroComment &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::CommentIndent);
Q_DECLARE_METATYPE(pas::ast::generic::IsMacroComment);
