#pragma once
#include <string>
#include <variant>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/elf_symtab.hpp"

namespace pepp::tc {

// Per-architecture assembler configuration.
struct RISCVDriverConfig {};
struct Pep10DriverConfig {};
using DriverConfig = std::variant<RISCVDriverConfig, Pep10DriverConfig>;

struct DriverResult {
  ElfResult elf;
  DiagnosticTable diagnostics;
  bool ok() const { return diagnostics.count() == 0; }
};

// Assembles `source` according to `config`, dispatching to the matching per-architecture pipeline
// (asmb_driver_riscv.cpp / asmb_driver_pep10.cpp).
DriverResult assemble(const DriverConfig &config, std::string source);

// Per-architecture entry points, for callers that already know the target.
DriverResult assemble_riscv(const RISCVDriverConfig &config, std::string source);
DriverResult assemble_pep10(const Pep10DriverConfig &config, std::string source);

} // namespace pepp::tc
