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
#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/compile/diagnostic/severity.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::link {
using tc::Severity;

// Index of a symbol in InputSymbols. 0 is STN_UNDEF.
using InputSymbolID = pepp::OpaqueHandle<struct InputSymbolTag, u32>;
consteval void allow_opaque_handle_add(InputSymbolID);
consteval void allow_opaque_handle_increment(InputSymbolID);

// Procedural errors encountered during the linking process.
enum class LinkCode : u8 {
  // No input provides an ELF file from which to take the class and byte order.
  NoInputs,
  // A config field or input kind that is not implemented yet.
  Unsupported,
};

// Errors during symbol resolution.
enum class SymbolCode : u8 {
  // sh_link does not point to a valid string table.
  BadStringTable,
  // sh_info is greater than the number of symbols in the table.
  BadFirstGlobal,
  LocalAfterFirstGlobal,
  GlobalBeforeFirstGlobal,
  // A non-local STT_FILE or STT_SECTION.
  LocalOnlyType,
  // Two strong definitions.
  MultipleDefinition,
  // A symbol is defined as TLS in one file and not in another.
  TlsMismatch,
  // A strong reference to an undefined symbol.
  Undefined,
};

// Errors could stem from the linker itself, our attempt to open files as a group or archive.
using DiagnosticCode = std::variant<LinkCode, bts::InputGroupError::Code, bts::ArchiveError::Code, SymbolCode>;
std::string_view to_string(const DiagnosticCode &code);

// Index of an input in LinkConfig::inputs, for problems found before the input has a file ID.
using ConfigInputID = pepp::OpaqueHandle<struct ConfigInputTag, u32>;
// A byte offset within an input file
struct ConfigInputOffset {
  ConfigInputID input;
  u32 offset = 0;
  bool operator==(const ConfigInputOffset &) const = default;
};

struct SymbolConflict {
  InputSymbolID symbol, first;
  bool operator==(const SymbolConflict &) const = default;
};

// What entity caused the error? monostate means the diagnostic can't be narrowed down to an item like a symbol or a
// file offset.
using DiagnosticSubject =
    std::variant<std::monostate, ConfigInputID, ConfigInputOffset, bts::InputElfSymbolHandle, InputSymbolID,
                 SymbolConflict>;

struct Diagnostic {
  Severity severity;
  DiagnosticCode code;
  DiagnosticSubject subject = {};
  bool operator==(const Diagnostic &) const = default;
};

// Errors and warnings sorted by insertion order.
class Diagnostics {
public:
  void add(Diagnostic diagnostic) { _entries.push_back(std::move(diagnostic)); }
  std::size_t size() const noexcept { return _entries.size(); }
  // Number of diagnostics at or above a given severity.
  std::size_t count(Severity minimum) const;
  auto begin() const { return _entries.cbegin(); }
  auto end() const { return _entries.cend(); }

private:
  std::vector<Diagnostic> _entries;
};
} // namespace pepp::link
