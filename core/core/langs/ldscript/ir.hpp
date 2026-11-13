/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/compile/source/location.hpp"
#include "core/formats/elf/enums_eheader.hpp"
#include "core/integers.h"
#include "core/langs/expr/ir.hpp"

// IR for GNU ld linker scripts.
namespace pepp::tc::ldscript {

// Symbol whose address is the program's entry point (e.g., ENTRY(_start)).
struct Entry {
  std::string symbol;
  support::LocationInterval source_interval;
};

// Files to link as if they were named on the command line (e.g., INPUT(a.o), GROUP(libc.a)).
// GROUP behaves like INPUT, since the linker already resolves archive members in any order.
struct InputFiles {
  enum class Kind : u8 { Input, Group } kind = Kind::Input;
  std::vector<std::string> files;
  support::LocationInterval source_interval;
};

// ELF format of the output file (e.g., OUTPUT_FORMAT(elf32-littleriscv)
struct OutputFormat {
  // A BFD target name (e.g., elf32-littleriscv) converted to ELF header values. EM_NONE represents unset or invalid
  // names.
  struct Format {
    bts::ElfClass elf_class = bts::ElfClass::ELFCLASS32;
    bts::ElfEncoding encoding = bts::ElfEncoding::ELFDATA2LSB;
    bts::ElfMachineType machine = bts::ElfMachineType::EM_NONE;
    bool operator==(const Format &) const = default;
  };
  // Default format, with optional big- and little-endian variants.
  Format format, big, little;
  support::LocationInterval source_interval;
};
// Return the BFD target name of format, or an empty string if it is unknown.
std::string_view to_string(const OutputFormat::Format &format);
// Convert a BFD target name to a Format, or nullopt if the name is unknown.
std::optional<OutputFormat::Format> output_format(std::string_view name);

// Machine type of the output file (e.g., OUTPUT_ARCH(riscv)).
struct OutputArch {
  bts::ElfMachineType machine = bts::ElfMachineType::EM_NONE;
  support::LocationInterval source_interval;
};
// Return the BFD architecture name of machine, or an empty string if it is unknown.
std::string_view architecture_name(bts::ElfMachineType machine);
// Convert a BFD architecture name to a machine type, or nullopt if the name is unknown.
std::optional<bts::ElfMachineType> architecture(std::string_view name);

// A symbol assignment (e.g., x = 4, x += 4, PROVIDE(x = .)).
struct Assignment {
  enum class Kind : u8 { Plain, Provide, ProvideHidden, Hidden } kind = Kind::Plain;
  std::string symbol;
  // Operator of a compound assignment (e.g., Add for +=), or nullopt for =.
  std::optional<expr::BinaryOp> compound;
  expr::Tree value;
  support::LocationInterval source_interval;
};
// Return the keyword for the assignment (e.g., PROVIDE), or an empty string for Plain.
std::string_view to_string(Assignment::Kind kind);
// Nullopt if not a valid assignment keyword
std::optional<Assignment::Kind> assignment_kind(std::string_view keyword);

// Input sections selected by file and section globs (e.g., *(.text .text.*), KEEP(crt0.o(.init))).
struct InputSectionDescription {
  std::string file;
  std::vector<std::string> sections;
  bool keep = false;
  support::LocationInterval source_interval;
};

// An output section with an optional address and its contents (e.g., .text 0x100 : { *(.text) }).
struct OutputSectionDescription {
  using Item = std::variant<Assignment, InputSectionDescription>;
  std::string name;
  std::optional<expr::Tree> address;
  std::vector<Item> items;
  support::LocationInterval source_interval;
};

// The SECTIONS command, which maps input sections into output sections.
struct Sections {
  using Item = std::variant<Assignment, OutputSectionDescription>;
  std::vector<Item> items;
  support::LocationInterval source_interval;
};

// A parsed or built-in linker script.
struct Script {
  using Command = std::variant<Entry, InputFiles, OutputFormat, OutputArch, Assignment, Sections>;
  std::vector<Command> commands;
};

// Print the script in GNU ld's strict syntax, which matches the output of ld --verbose.
std::string to_string(const Script &script);

} // namespace pepp::tc::ldscript
