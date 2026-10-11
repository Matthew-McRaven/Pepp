#pragma once
#include <memory>
#include <optional>
#include <span>
#include <stack>
#include <unordered_set>
#include <utility>
#include <vector>
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/conditionals.hpp"
#include "core/langs/asmb/ir_program.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/langs/asmb_riscv/lexer.hpp"

/*
 * N= { <statement>, <line>, <instruction>, <directive>, <mem_arg>, <operand> }
 * T= { INTEGER, IDENTIFIER, SYMBOL_DECL, DOT_CMD, COMMENT, EMPTY, STRING, CHAR }
 *    REGISTER ⊂ IDENTIFIER (valid x0–x31 register, arch or ABI name)
 *    Literal tokens: COMMA=",", LPAREN="(", RPAREN=")"
 * P= the productions
 *   1. <mem_arg>     → INTEGER LPAREN REGISTER RPAREN
 *   2. <operand>     → REGISTER | INTEGER | IDENTIFIER | STRING | CHAR | <mem_arg>
 *   3. <instruction> → IDENTIFIER <operands>  -- operand pattern is mnemonic-driven:
 *      R-type:        REGISTER COMMA REGISTER COMMA REGISTER
 *      I-arith/shift: REGISTER COMMA REGISTER COMMA INTEGER
 *      I-load / S:    REGISTER COMMA <mem_arg>
 *      B-type:        REGISTER COMMA REGISTER COMMA <operand>
 *      U-type:        REGISTER COMMA INTEGER
 *      J-type:        REGISTER COMMA <operand>
 *      pseudo:        (<operand> (COMMA <operand>)*)?
 *   4. <directive>   → DOT_CMD (<operand> (COMMA <operand>)*)?
 *   5. <line>        → (<instruction> | <directive>) [COMMENT]
 *   6. <statement>   → [COMMENT | SYMBOL_DECL <line> | SYMBOL_DECL [COMMENT]] EMPTY
 * S= <statement>
 */
namespace pepp {

namespace core::symbol {
class LeafTable;
}
namespace tc {
namespace expr {
struct Parsed;
}

class DiagnosticTable;
namespace parser {
// Raw operand as parsed before being converted to IR.
struct RISCVOperand {
  std::shared_ptr<const pepp::tc::expr::Parsed> expression;
  // If true: 0, (x3). If false 0(x3)
  bool comma_before = false;
};

struct RISCVParser {
  RISCVParser(support::SeekableData &&data);

  IRProgram parse(DiagnosticTable &);
  std::shared_ptr<pepp::core::symbol::LeafTable> symbol_table() const;

  void debug_print_tokens(bool debug);

private:
  using OptionalSymbol = std::optional<std::shared_ptr<pepp::core::symbol::Entry>>;
  std::shared_ptr<pepp::ast::IRValue> argument();
  std::shared_ptr<pepp::ast::IRValue> numeric_argument();
  std::shared_ptr<pepp::ast::IRValue> hex_argument();
  std::shared_ptr<pepp::ast::Symbolic> identifier_argument();
  // Every operand written after a mnemonic, each parsed as an expression.
  std::vector<RISCVOperand> mnemonic_operands();
  // Match the parsed operands to one of the valid instruction patterns for that mnemonic.
  std::shared_ptr<pepp::tc::IntegerInstruction> match_alternative(const riscv::Mnemonic &entry,
                                                                  std::span<const RISCVOperand> ops);
  std::shared_ptr<pepp::tc::IntegerInstruction> instruction();
  std::shared_ptr<pepp::tc::LinearIR> pseudo(OptionalSymbol symbol);
  std::shared_ptr<pepp::tc::LinearIR> line(OptionalSymbol symbol);
  std::shared_ptr<pepp::tc::LinearIR> statement();

  void synchronize();

  std::shared_ptr<std::unordered_set<std::string>> _pool;
  // The lexer for the text being parsed, either the source or a macro body expanded within it.
  lex::Buffer *active_buffer() { return _lexer_stack.top().second.get(); }
  pepp::langs::RISCVLexer *active_lexer() { return _lexer_stack.top().first.get(); }
  std::stack<std::pair<std::shared_ptr<pepp::langs::RISCVLexer>, std::shared_ptr<lex::Buffer>>> _lexer_stack;
  std::shared_ptr<pepp::langs::RISCVLexer> _root_lexer;
  std::shared_ptr<pepp::core::symbol::LeafTable> _symtab;
  // The location counter (`.`) of the line being parsed, created on first use.
  std::shared_ptr<pepp::core::symbol::Entry> _location_counter;
  Conditionals _conditionals;
};
} // namespace parser
} // namespace tc
} // namespace pepp
