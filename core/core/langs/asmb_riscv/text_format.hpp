#pragma once
#include "core/langs/asmb/ir_program.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/math/bitmanip/span.hpp"
#include "ir_attributes.hpp"

namespace pepp::tc {
namespace ir {
struct LinearIR;
}
struct ProgramObjectCodeResult;

// Relative sizes for each column in the listing.
struct RISCVFormatOptions {
  static constexpr int col0_width = 9;  // Symbol declaration
  static constexpr int col1_width = 8;  // Mnemonic or dot command
  static constexpr int col2_width = 18; // Operands, dot command arguments
};

// Helper which formats 4 columns of text using the default column widths for RISC-V.
// Inserts padding between columns when they bleed into each other, and trims trailing spaces.
std::string riscv_format_as_columns(const std::string &col0, const std::string &col1, const std::string &col2,
                                    const std::string &col3);

// Format a single IR line as RISC-V assembly source.
std::string riscv_format_source(const LinearIR *line);
// Format every line of a program as source code with one string per line.
std::vector<std::string> riscv_format_source(const IRProgram &program);

// Format a line as a listing (address + object bytes + source code). Keep 4 bytes of object code per row, with
// continuation rows for directives that generate more bytes.
std::vector<std::string> riscv_format_listing(const LinearIR *line,
                                              const IRMemoryAddressTable<RISCVAddress> &addresses,
                                              const ProgramObjectCodeResult &object_code);
std::vector<std::string> riscv_format_listing(const IRProgram &program,
                                              const IRMemoryAddressTable<RISCVAddress> &addresses,
                                              const ProgramObjectCodeResult &object_code);
} // namespace pepp::tc
