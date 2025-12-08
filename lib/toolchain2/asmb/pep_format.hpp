#pragma once
#include <QString>
#include "./pep_common.hpp"
#include "utils/bits/span.hpp"

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
QString format_as_columns(const QString &col0, const QString &col1, const QString &col2, const QString &col3);
// If you modify this function, you must also modify SourceVisitor.
// You must then also modify the tests proving equivalence between IR and token formatting.
//
// Format a sequence of tokens as Pep/N assembly source code.
// It does not make semantic checks that the code is correct, and can be used in more cases than assemble+format.
// If the line is not valid, it will return an empty string. In this case, you will need to reach into the tokenizer and
// grab the original source text for the source interval.
QString format_source(bits::span<std::shared_ptr<lex::Token> const> tokens);

// Format a single IR line as if by format_source(<tokens>).
QString format_source(const ir::LinearIR *line);

// Format a single line
QStringList format_listing(const ir::LinearIR *line, const IRMemoryAddressTable &addresses,
                           const ProgramObjectCodeResult &object_code);
QStringList format_listing(const PepIRProgram &program, const IRMemoryAddressTable &addresses,
                           const ProgramObjectCodeResult &object_code);
} // namespace pepp::tc
