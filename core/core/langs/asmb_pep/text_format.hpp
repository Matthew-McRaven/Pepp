#pragma once
#include "core/langs/asmb/ir_program.hpp"
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
// Split s at the first item matching predicate. That item is dropped, while first holds the items before it and second
// holds the items after it.
template <typename T, typename F> std::pair<std::span<T>, std::span<T>> split_exclusive(std::span<T> s, F predicate) {
  for (std::size_t i = 0; i < s.size(); ++i)
    if (predicate(s[i])) return {s.first(i), s.subspan(i + 1)};
  return {s, std::span<T>{}};
}

// Format a single IR line as Pep/N assembly source code.
std::string format_source(const LinearIR *line);
std::vector<std::string> format_source(const IRProgram &program);

// Format a single line
std::vector<std::string> format_listing(const LinearIR *line, const IRMemoryAddressTable<PeppAddress> &addresses,
                                        const ProgramObjectCodeResult &object_code);
std::vector<std::string> format_listing(const IRProgram &program, const IRMemoryAddressTable<PeppAddress> &addresses,
                                        const ProgramObjectCodeResult &object_code);
} // namespace pepp::tc
