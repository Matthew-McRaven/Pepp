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
#include <fmt/format.h>
#include <string>
#include <variant>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/link_config.hpp"

namespace pepp::link {
// Returns the name of a file matching ld's formatting. An in-memory file has no path and prints as <memory>.
template <bts::ElfBits B, bts::ElfEndian E>
std::string file_name(const bts::PackedInputElfGroup<B, E> &group, bts::ElfFileID file) {
  const auto &provenance = group.provenance(file);
  if (const auto *path = std::get_if<bts::ElfFromPath>(&provenance)) return path->path.string();
  else if (const auto *member = std::get_if<bts::ElfFromArchive>(&provenance)) {
    const auto &archive = group.archive(member->archive);
    return fmt::format("{}({})", archive.path() ? archive.path()->string() : "<memory>", archive.name(member->member));
  }
  return "<memory>";
}

std::string input_file_name(const decltype(LinkInput::source) &source);
} // namespace pepp::link
