#pragma once
#include <memory>
#include <stack>
#include <unordered_set>
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/macro/macro_replacement.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/ir_program.hpp"

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
namespace pepp {

namespace core::symbol {
class LeafTable;
}
namespace tc {
namespace lex {
class PepLexer;
}
class DiagnosticTable;
namespace parser {
struct PepParser {
  PepParser(support::SeekableData &&data, std::shared_ptr<pepp::tc::MacroRegistry> reg);

  IRProgram parse(DiagnosticTable &);
  std::shared_ptr<pepp::core::symbol::LeafTable> symbol_table() const;

  void debug_print_tokens(bool debug);

private:
  using OptionalSymbol = std::optional<std::shared_ptr<pepp::core::symbol::Entry>>;
  std::shared_ptr<pepp::ast::IRValue> argument();
  std::shared_ptr<pepp::ast::IRValue> numeric_argument();
  std::shared_ptr<pepp::ast::IRValue> hex_argument();
  std::shared_ptr<pepp::ast::Symbolic> identifier_argument();
  std::shared_ptr<LinearIR> instruction();
  std::shared_ptr<LinearIR> macro(DiagnosticTable &, OptionalSymbol symbol);
  std::shared_ptr<LinearIR> pseudo(OptionalSymbol symbol);
  std::shared_ptr<LinearIR> line(DiagnosticTable &, OptionalSymbol symbol);
  std::shared_ptr<LinearIR> statement(DiagnosticTable &);
  // Body which actually does the parsing until the top lexer in _lexer_stack is exhausted.
  // If root_loc is nullopt, the location from the PepParserError and underlying IR will be left untouched.
  // Otherwise, all "source" locations will be updated to point to root_loc.
  IRProgram do_parse(DiagnosticTable &diag, std::optional<support::LocationInterval> root_loc);

  void synchronize();

  std::shared_ptr<std::unordered_set<std::string>> _pool;
  lex::Buffer *active_buffer() { return _lexer_stack.top().second.get(); }
  lex::PepLexer *active_lexer() { return _lexer_stack.top().first.get(); }
  std::stack<std::pair<std::shared_ptr<lex::PepLexer>, std::shared_ptr<lex::Buffer>>> _lexer_stack;
  std::shared_ptr<lex::PepLexer> _root_lexer;
  std::shared_ptr<pepp::core::symbol::LeafTable> _symtab;
  std::shared_ptr<pepp::tc::MacroRegistry> _macros;

  MacroCounters _counters;
  struct ConditionalStack {
    bool matched_any = false; // True if any conditional guard has been meet at this level. Used to prevent selecting
                              // further elseif/else blocks
    bool matched_this_stmt = false; // Only true in the conditional block which first matches is guard
    bool matched_else = false;      // Prevent matching an elseif after an else
  };
  std::vector<ConditionalStack> _conditionals;
  // if non-zero, takes precedence over _conditionals until 0.
  // incremented when macro returns non-nullptr, decremented by the skip loop in statement
  u16 _active_macro_defs = 0;
  // Skip mode is true when any element in _conditionals sets matched_this_stmt=false.
  bool skip_mode() const;
};
} // namespace parser
} // namespace tc
} // namespace pepp
