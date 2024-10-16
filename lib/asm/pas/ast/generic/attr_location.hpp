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
struct Location {
  qsizetype line = 0;
  bool valid = false;
  bool operator==(const Location &other) const = default;
};

struct SourceLocation {
  static const inline QString attributeName = "generic:source_loc";
  static const inline uint8_t attribute = 11;
  Location value = {}; // The location (line) line in the source file on which
                       // the node starts.
  bool operator==(const SourceLocation &other) const = default;
};

struct RootLocation {
  static const inline QString attributeName = "generic:root_loc";
  static const inline uint8_t attribute = 30;
  Location value = {}; // The line in the root source file to which this belongs
  bool operator==(const RootLocation &other) const = default;
};

struct ListingLocation {
  static const inline QString attributeName = "generic:listing_loc";
  static const inline uint8_t attribute = 31;
  Location value = {}; // The line in the root source file to which this belongs
  bool operator==(const ListingLocation &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::SourceLocation);
Q_DECLARE_METATYPE(pas::ast::generic::RootLocation);
Q_DECLARE_METATYPE(pas::ast::generic::ListingLocation);
