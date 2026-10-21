#pragma once
#include "core/langs/asmb/ir_program.hpp"
#include "core/langs/asmb/listing.hpp"
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

// Bytes of object code per listing row. Directives that generate more bytes continue onto extra rows.
static constexpr int riscv_listing_bytes_per_line = 4;
inline constexpr ListingStyle riscv_listing_style{.address_width = 8, .bytes_per_row = riscv_listing_bytes_per_line};
} // namespace pepp::tc
