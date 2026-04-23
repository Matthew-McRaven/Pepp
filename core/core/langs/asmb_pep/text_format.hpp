#pragma once
#include "core/langs/asmb/ir_program.hpp"
#include "core/math/bitmanip/span.hpp"
#include "ir_attributes.hpp"

namespace pepp::tc {
namespace lex {
struct Token;
}
namespace ir {
struct LinearIR;
}
struct ProgramObjectCodeResult;

// Relative sizes for each "column" in the listing.
struct FormatOptions {
  static constexpr int col0_width = 9;  // Symbol Declaration
  static constexpr int col1_width = 8;  // Mnemonics, dot commands, macro
  static constexpr int col2_width = 12; // Operand specifier + addressing mode, dot command arguments, macro arguments
};

// Helper which formats 4 columns of text using the default column width for pep/10.
// Insert padding betweens columns when they bleed in to each other, and trims right spaces.
std::string format_as_columns(const std::string &col0, const std::string &col1, const std::string &col2,
                              const std::string &col3);
// If you modify this function, you must also modify SourceVisitor.
// You must then also modify the tests proving equivalence between IR and token formatting.
//
// Format a sequence of tokens as Pep/N assembly source code.
// It does not make semantic checks that the code is correct, and can be used in more cases than assemble+format.
// If the line is not valid, it will return an empty string. In this case, you will need to reach into the tokenizer and
// grab the original source text for the source interval.
std::string format_source(bits::span<std::shared_ptr<lex::Token> const> tokens);

// Split a span of tokens into a head+rest pair, where head includes all items up-to and including the first item
// matching the predicate.
// Useful for splitting token streams on newline boundaries.
template <typename T, typename F> std::pair<std::span<T>, std::span<T>> split_inclusive(std::span<T> s, F predicate) {
  for (std::size_t i = 0; i < s.size(); ++i)
    if (predicate(s[i])) return {s.first(i + 1), s.subspan(i + 1)};
  return {s, std::span<T>{}};
}
// Same as split_inclusive, but useful for excluding the newline boundaries.
template <typename T, typename F> std::pair<std::span<T>, std::span<T>> split_exclusive(std::span<T> s, F predicate) {
  for (std::size_t i = 0; i < s.size(); ++i)
    if (predicate(s[i])) return {s.first(i), s.subspan(i + 1)};
  return {s, std::span<T>{}};
}

// Format a single IR line as if by format_source(<tokens>).
std::string format_source(const LinearIR *line);

// Format a single line
std::vector<std::string> format_listing(const LinearIR *line, const IRMemoryAddressTable<PeppAddress> &addresses,
                                        const ProgramObjectCodeResult &object_code);
std::vector<std::string> format_listing(const IRProgram &program, const IRMemoryAddressTable<PeppAddress> &addresses,
                                        const ProgramObjectCodeResult &object_code);
} // namespace pepp::tc
