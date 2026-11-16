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
#include "core/compile/source/location.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::link {
using tc::Severity;

// Index of a symbol in InputSymbols. 0 is STN_UNDEF.
using InputSymbolID = pepp::OpaqueHandle<struct InputSymbolTag, u32>;
consteval void allow_opaque_handle_add(InputSymbolID);
consteval void allow_opaque_handle_increment(InputSymbolID);

// Errors from the linker's own passes.
enum class LinkCode : u8 {
  // No input provides an ELF file from which to take the class and byte order.
  NoInputs,
  // A linker script failed to parse. The subject is a ScriptMessage.
  BadScript,
  // A pass received a linker script not parsed by parse_scripts().
  UnparsedScript,
  // A linker script expression could not be evaluated. The subject is a ScriptMessage.
  BadExpression,
  // An assignment moved the location counter backwards within an output section. The subject is a ScriptMessage.
  LocationCounterBackwards,
  // The entry symbol is not defined, so the entry point is another address. The subject is an EntryFallback.
  NoEntrySymbol,
  // The output file could not be created or mapped. The subject is a ScriptMessage with the offending file.
  CannotWriteOutput,
  // The output sections cannot be arranged into segments. The subject is a ScriptMessage with the output file.
  BadLayout,
  // An address, size, or entry point does not fit in the output's ELF class. The subject is a ScriptMessage.
  AddressOverflow,
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

// Errors while applying relocations. The subject is a RelocationSite.
enum class RelocationCode : u8 {
  NotRela,
  // The relocation type is unknown for the file's machine.
  UnsupportedType,
  // The value does not fit in the bytes that the relocation writes.
  Truncated,
  // r_offset is outside of the section.
  BadOffset,
  // The symbol has no address (e.g., it is in a section that is not part of the output).
  NoSymbolAddress,
};

using DiagnosticCode =
    std::variant<LinkCode, bts::InputGroupError::Code, bts::ArchiveError::Code, SymbolCode, RelocationCode>;
std::string_view to_string(const DiagnosticCode &code);

// Index of an input in LinkConfig::inputs, for problems found before the input has a file ID.
using ConfigInputID = pepp::OpaqueHandle<struct ConfigInputTag, u32>;
// A byte offset within an input file
struct ConfigInputOffset {
  ConfigInputID input;
  u32 offset = 0;
  bool operator==(const ConfigInputOffset &) const = default;
};

// A problem with a linker script, which has no ConfigInputID. If location is invalid, the problem is with the whole
// file. If message is empty, the diagnostic's code describes the problem.
struct ScriptMessage {
  std::string file;
  tc::support::LocationInterval location;
  std::string message;
  bool operator==(const ScriptMessage &) const = default;
};
struct SymbolConflict {
  InputSymbolID symbol, first;
  bool operator==(const SymbolConflict &) const = default;
};

// An entry point chosen because the entry symbol is missing.
struct EntryFallback {
  std::string symbol;
  u64 address = 0;
  bool operator==(const EntryFallback &) const = default;
};

// The target offset of a relocation applied to an input section. For NotRela, section is the relocation section.
struct RelocationSite {
  bts::InputElfSectionHandle section;
  u64 offset = 0;
  bool operator==(const RelocationSite &) const = default;
};

// What entity caused the error? monostate means the diagnostic can't be narrowed down to an item like a symbol or a
// file offset.
using DiagnosticSubject = std::variant<std::monostate, ConfigInputID, ConfigInputOffset, ScriptMessage,
                                       bts::InputElfSymbolHandle, InputSymbolID, SymbolConflict, EntryFallback,
                                       RelocationSite>;

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
  std::size_t count(Severity minimum) const;
  auto begin() const { return _entries.cbegin(); }
  auto end() const { return _entries.cend(); }

private:
  std::vector<Diagnostic> _entries;
};
} // namespace pepp::link
