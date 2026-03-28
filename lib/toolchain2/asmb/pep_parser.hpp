#pragma once
#include <memory>
#include "./pep_common.hpp"
#include "./pep_ir.hpp"
#include "./pep_lexer.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/lex/buffer.hpp"
#include "toolchain2/asmb/common_diag.hpp"

/*
 * N= { <argument>, <instruction>, <line>, <pseudo>, <statement> }
 * T= { HEX, DECICMAL, COMMA, EMPTY, IDENTIFIER, SYMBOL, DOT}
 * P= the productions
 *   1. <argument> -> HEX | DECICMAL | IDENTIFIER | STRING | CHAR
 *   2. <instruction> -> IDENTIFIER [<argument> [COMMA IDENTIFIER]]
 *   3. <pseudo> -> DOT <argument>
 *   4. <macro> - > MACRO <argument> (COMMA ARGUMENT)*
 *   5. <line> -> (<pseudo> | <instruction> | <macro>) [COMMENT]
 *   6. <statement> →[COMMENT | [SYMBOL] <line>] EMPTY
 * S= <statement>
 */
namespace pepp::tc::parser {
struct PepParser {
  PepParser(support::SeekableData &&data);

  PepIRProgram parse(DiagnosticTable &);
  std::shared_ptr<pepp::core::symbol::LeafTable> symbol_table() const;

  void debug_print_tokens(bool debug);

private:
  using OptionalSymbol = std::optional<std::shared_ptr<pepp::core::symbol::Entry>>;
  std::shared_ptr<pepp::ast::IRValue> argument();
  std::shared_ptr<pepp::ast::IRValue> numeric_argument();
  std::shared_ptr<pepp::ast::IRValue> hex_argument();
  std::shared_ptr<pepp::ast::Symbolic> identifier_argument();
  std::shared_ptr<ir::LinearIR> instruction();
  std::shared_ptr<ir::LinearIR> pseudo(OptionalSymbol symbol);
  std::shared_ptr<ir::LinearIR> line(OptionalSymbol symbol);
  std::shared_ptr<ir::LinearIR> statement();

  void synchronize();

  std::shared_ptr<std::unordered_set<std::string>> _pool;
  std::shared_ptr<lex::PepLexer> _lexer;
  std::shared_ptr<lex::Buffer> _buffer;
  // Must be QSharedPointer until old toolchain has been entirely ported.
  std::shared_ptr<pepp::core::symbol::LeafTable> _symtab;
};
} // namespace pepp::tc::parser
