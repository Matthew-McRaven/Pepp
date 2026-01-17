/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

namespace pepp::bts {

// A fixup which requires no additional parameters to function.
// If there is a dependency between fixups, you must order them manually.
// They are used to solve problems of the following type:
//   You need a _DYNAMIC symbol that points to the start of the .dynamic section.
//   To know the start of the .dynamic section, the file needs to be laid out.
//   However, adding the _DYNAMIC symbol changes the layout.
// This class can be used to break that dependency loop for fixed-size types using the following pattern.
//   Allocate the _DYNAMIC symbol, set its value to 0.
//   Create a function which assigns _DYNAMIC's value to the sh_addr of the .dynamic section.
//   ...
//   Perform layout, and apply all fixups.
// This is almost certainly the worst possible API, and I should be working towards something that looks more like
// relocations.
struct AbsoluteFixup {
  std::function<void()> update;
};

} // namespace pepp::bts
