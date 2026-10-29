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
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/ar/packed_archive.hpp"

namespace pepp::link {

// Input files to the linker in command-line order.
struct LinkInput {
  struct Script {
    std::variant<std::filesystem::path, std::string> source;
  };

  std::variant<std::filesystem::path, bts::PackedInputArchive, Script, bts::AnyInputElf> source;
};

// Configuration options for the linker that are shared between command-line and library usage.
struct LinkConfig {
  std::vector<LinkInput> inputs;
  // -o
  std::optional<std::filesystem::path> output;
  // -e / --entry
  std::optional<std::string> entry;
  // --section-start=name=address, and -Ttext as .text.
  std::vector<std::pair<std::string, u64>> section_start;
  // -m. Set the class+byte-order before reading input files.
  std::optional<std::string> emulation;
};
} // namespace pepp::link
