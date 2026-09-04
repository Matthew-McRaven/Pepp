#pragma once
#include <string>
#include <variant>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/elf_symtab.hpp"

namespace pepp::tc {

struct ListingConfig {
  bool omit_false_conditionals = false;
  bool omit_debugging_directives = false;
  bool include_macro_expansions = false;
};
struct FormattingConfig {
  ListingConfig listing_config;
  // If non-null, call this function with the corresponding lines of the source / listing.
  // Signature allows the caller to determine where the text ends up (e.g., in memory, stdout, temporary file).
  using format_to = std::function<void(std::vector<std::string> &&)>;
  format_to listing_format = nullptr;
  format_to source_format = nullptr;
};

// Per-architecture assembler configuration.
struct RISCVDriverConfig {};
struct Pep10DriverConfig {};
using DriverConfig = std::variant<RISCVDriverConfig, Pep10DriverConfig>;

struct DriverResult {
  ElfResult elf;
  DiagnosticTable diagnostics;
  bool ok() const { return diagnostics.count() == 0; }
};

// Dispatch to the assembler based on the target architecture.
DriverResult assemble(const DriverConfig &config, const FormattingConfig &, std::string source);

// Per-architecture entry points, for callers that already know the target.
DriverResult assemble_riscv(const RISCVDriverConfig &config, const FormattingConfig &, std::string source);
DriverResult assemble_pep10(const Pep10DriverConfig &config, const FormattingConfig &, std::string source);

} // namespace pepp::tc
