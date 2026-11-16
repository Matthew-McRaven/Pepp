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
#include "core/link/entry.hpp"
#include <variant>

u64 pepp::link::code_section_address(const tc::ldscript::Sections &sections) {
  for (const auto &command : sections.commands)
    if (const auto *output = std::get_if<tc::ldscript::OutputSectionDescription>(&command))
      if (output->name == ".text") return output->vma;
  // TODO: locate non-.text code sections via output section flags.
  return 0;
}
