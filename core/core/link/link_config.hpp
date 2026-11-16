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
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/langs/ldscript/ir.hpp"

namespace pepp::link {

// Input files to the linker in command-line order.
struct LinkInput {
  // A linker script provided via -T/--script or one built in C++ (e.g., the default script).
  struct Script {
    std::variant<std::filesystem::path, tc::ldscript::Script> source;
  };
  // A file already mapped into memory. Replaces a std::filesystem::path on first open.
  struct Opened {
    std::filesystem::path path;
    bts::OpenedInput file;
  };

  std::variant<std::filesystem::path, bts::PackedInputArchive, Script, bts::AnyInputElf, Opened> source;
  // Allow resolution of symbols in this file, but do not include its sections in the output.
  bool just_symbols = false;
};

// Linker options shared between command line and library usage.
struct LinkConfig {
  std::vector<LinkInput> inputs;
  // -o. The link writes temporary_output(output), overwriting any file there, then renames it to output.
  std::optional<std::filesystem::path> output;
  // -e / --entry
  std::optional<std::string> entry;
  // --section-start=name=address, and -Ttext as .text.
  std::vector<std::pair<std::string, u64>> section_start;
  // -m. Set the class+byte-order before reading input files.
  std::optional<std::string> emulation;
  // Archive file names whose symbols are ineligible for resolution. ALL excludes every archive.
  std::vector<std::string> exclude_libs;
  // If present, keep only these symbols. Otherwise, keep every symbol.
  std::optional<std::vector<std::string>> retain_symbols;
};

// TODO: Replace with the built-in Pep default script.
tc::ldscript::Script default_pep_script();
// Read whitespace-separated symbol names from a text file. Returns nullopt if the file cannot be read.
std::optional<std::vector<std::string>> read_symbol_list(const std::filesystem::path &path);
// Parse `name=address`, where address is hexadecimal. Returns nullopt if it is malformed.
std::optional<std::pair<std::string, u64>> parse_section_start(std::string_view arg);
} // namespace pepp::link
