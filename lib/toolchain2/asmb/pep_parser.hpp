#pragma once
#include <memory>
#include "./pep_ir.hpp"
#include "./pep_lexer.hpp"
#include "toolchain/symbol/table.hpp"
#include "toolchain2/support/allocators/string_pool.hpp"
#include "toolchain2/support/lex/buffer.hpp"

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

  std::vector<std::shared_ptr<tc::ir::LinearIR>> parse();
  QSharedPointer<symbol::Table> symbol_table() const;

private:
  std::shared_ptr<pas::ast::value::Base> argument();
  std::shared_ptr<pas::ast::value::Base> numeric_argument();
  std::shared_ptr<pas::ast::value::Base> identifier_argument();
  std::shared_ptr<ir::LinearIR> instruction();
  std::shared_ptr<ir::LinearIR> pseudo();
  std::shared_ptr<ir::LinearIR> line();
  std::shared_ptr<ir::LinearIR> statement();

  void synchronize();

  std::shared_ptr<support::StringPool> _pool;
  std::shared_ptr<lex::PepLexer> _lexer;
  std::shared_ptr<lex::Buffer> _buffer;
  // Must be QSharedPointer until old toolchain has been entirely ported.
  QSharedPointer<symbol::Table> _symtab;
};
} // namespace pepp::tc::parser
