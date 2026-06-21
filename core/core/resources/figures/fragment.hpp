/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace pepp {
class Figure;
class Fragment {
public:
  //! The programming language this element is written in
  std::string language;
  //! The textual contents of the element
  std::string contents() const;
  std::function<std::string()> contents_fn = Fragment::empty;
  //! The figure which contains this element. Needed to access default OS / test
  //! items.
  std::weak_ptr<Figure> figure;
  std::string name, copy_type;
  bool is_default = false, is_hidden = false;
  std::string export_path = "";

protected:
  static inline std::string empty() { return ""; }
  mutable std::optional<std::string> _contents = std::nullopt;
};

class Test {
public:
  std::string input, output;
};

} // namespace pepp