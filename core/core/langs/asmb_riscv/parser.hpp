#pragma once
#include <memory>
#include <optional>
#include <unordered_set>
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/langs/asmb_riscv/ir_program.hpp"
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
 *   6. <statement>   → [COMMENT | [SYMBOL_DECL] <line>] EMPTY
 * S= <statement>
 */
namespace pepp {

namespace core::symbol {
class LeafTable;
}
namespace tc {

class DiagnosticTable;
namespace parser {
struct RISCVParser {
  RISCVParser(support::SeekableData &&data);

  RISCVIRProgram parse(DiagnosticTable &);
  std::shared_ptr<pepp::core::symbol::LeafTable> symbol_table() const;

  void debug_print_tokens(bool debug);

private:
  using OptionalSymbol = std::optional<std::shared_ptr<pepp::core::symbol::Entry>>;
  std::optional<u8> register_integer();
  std::shared_ptr<pepp::ast::IRValue> argument();
  std::shared_ptr<pepp::ast::IRValue> numeric_argument();
  std::shared_ptr<pepp::ast::IRValue> hex_argument();
  std::shared_ptr<pepp::ast::Symbolic> identifier_argument();
  std::shared_ptr<pepp::tc::IntegerInstruction> instruction();
  // std::shared_ptr<pepp::tc::LinearIR> pseudo(OptionalSymbol symbol);
  std::shared_ptr<pepp::tc::LinearIR> line(OptionalSymbol symbol);
  std::shared_ptr<pepp::tc::LinearIR> statement();

  void synchronize();

  std::shared_ptr<std::unordered_set<std::string>> _pool;
  std::shared_ptr<pepp::langs::RISCVLexer> _lexer;
  std::shared_ptr<lex::Buffer> _buffer;
  std::shared_ptr<pepp::core::symbol::LeafTable> _symtab;
};
} // namespace parser
} // namespace tc
} // namespace pepp
