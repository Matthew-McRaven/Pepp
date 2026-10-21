#pragma once
#include "core/arch/pepx/asmb/pep_dialect.hpp"
#include "core/langs/asmb/ir_program.hpp"
#include "core/langs/asmb/listing.hpp"
#include "core/math/bitmanip/span.hpp"
#include "ir_attributes.hpp"

namespace pepp::tc {
namespace ir {
struct LinearIR;
}
struct ProgramObjectCodeResult;

// Relative sizes for each column in the listing.
struct FormatOptions {
  static constexpr int col0_width = 9;  // Symbol Declaration
  static constexpr int col1_width = 8;  // Mnemonics, dot commands, macro
  static constexpr int col2_width = 12; // Operand specifier + addressing mode, dot command arguments, macro arguments
};

// Helper which formats 4 columns of text using the default column width for pep/10.
// Insert padding betweens columns when they bleed in to each other, and trims right spaces.
std::string format_as_columns(const std::string &col0, const std::string &col1, const std::string &col2,
                              const std::string &col3);

// Format a single IR line as Pep/N assembly source code.
std::string format_source(const pep::Dialect &dialect, const LinearIR *line);
std::vector<std::string> format_source(const pep::Dialect &dialect, const IRProgram &program);

// Bytes of object code per listing row. Longer lines continue onto extra rows.
static constexpr int pep_listing_bytes_per_line = 3;
inline constexpr ListingStyle pep_listing_style{.address_width = 4, .bytes_per_row = pep_listing_bytes_per_line};
} // namespace pepp::tc
