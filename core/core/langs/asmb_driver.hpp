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
#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "core/compile/macro/macro_registry.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/listing.hpp"

namespace pepp::tc {

struct ListingConfig {
  bool omit_false_conditionals = false;
  bool omit_debugging_directives = false;
  bool include_macro_expansions = false;
};
// The produce outputs besides the ELF.
struct FormattingConfig {
  ListingConfig listing_config;
  // Format the IR as source code or as a listing, if assembly succeeds;
  bool source = false, listing = false;
};

// Per-architecture assembler configuration.
struct RISCVDriverConfig {
  std::vector<std::pair<std::string, u32>> symdefs;
  std::shared_ptr<pepp::tc::MacroRegistry> macros = nullptr;
  DebugLineFiles debug_files;
};
struct Pep10DriverConfig {
  std::vector<std::pair<std::string, u32>> symdefs;
  std::shared_ptr<pepp::tc::MacroRegistry> macros = nullptr;
  DebugLineFiles debug_files;
};
// Pep/9 does not support macros.
struct Pep9DriverConfig {
  std::vector<std::pair<std::string, u32>> symdefs;
  DebugLineFiles debug_files;
};
using DriverConfig = std::variant<RISCVDriverConfig, Pep10DriverConfig, Pep9DriverConfig>;

class DriverResult {
public:
  DiagnosticTable diagnostics;
  bool ok() const { return diagnostics.count(Severity::Error) == 0; }
  // An ELF file suitable for linking+loading that is null unless ok().
  ElfResult *elf() { return ok() ? unchecked_elf() : nullptr; }
  // An ELF file generated despite errors, for debugging purposes and generating listings.
  // Always null after a fatal error, but may be non-null after non-fatal errors.
  ElfResult *unchecked_elf() { return _elf ? &*_elf : nullptr; }
  void set_elf(ElfResult elf) { _elf = std::move(elf); }

  // If parsing succeeded, the formatted source code of the input program. Else nullopt.
  std::optional<std::vector<std::string>> formatted_source;
  // A listing's text excluding object-code continuation lines. Must be combined with an ELF file to produce a full
  // listing.
  struct UnformattedListing {
    std::vector<std::string> text;
    ListingStyle style;
  };
  // Present if a listing was requested and an ELF was written, even if there were non-fatal errors.
  std::optional<UnformattedListing> unformatted_listing;
  // A properly formatted listing, using the elf file and unformatted_listing
  std::expected<std::vector<std::string>, std::string> listing();

private:
  std::optional<ElfResult> _elf;
};

// Dispatch to the assembler based on the target architecture.
DriverResult assemble(const DriverConfig &config, const FormattingConfig &, std::string source);

// Per-architecture entry points, for callers that already know the target.
DriverResult assemble_riscv(const RISCVDriverConfig &config, const FormattingConfig &, std::string source);
DriverResult assemble_pep10(const Pep10DriverConfig &config, const FormattingConfig &, std::string source);
DriverResult assemble_pep9(const Pep9DriverConfig &config, const FormattingConfig &, std::string source);

// A helper to copy command-line / external symbol definitions into a symtab as weak constants.
void inject_symdefs(std::vector<std::pair<std::string, u32>> symdefs, core::symbol::LeafTable &symtab, u8 byte_width);

} // namespace pepp::tc
