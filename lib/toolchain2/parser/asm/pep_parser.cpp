#include "./pep_parser.hpp"
#include "toolchain/pas/ast/value/character.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/ast/value/numeric.hpp"
#include "toolchain/pas/ast/value/string.hpp"

#include <toolchain/pas/ast/value/hexadecimal.hpp>
#include <toolchain2/ir/asm/attributes.hpp>
#include <toolchain2/lex/asm/pep_tokens.hpp>

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data)
    : _pool(std::make_shared<pepp::tc::support::StringPool>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(std::make_shared<symbol::Table>(2)) {}

std::vector<std::shared_ptr<pepp::tc::ir::LinearIR>> pepp::tc::parser::PepParser::parse() {
  std::vector<std::shared_ptr<ir::LinearIR>> lines;

  while (_buffer->input_remains()) {
    lex::Checkpoint cp(*_buffer);
    if (auto instr = instruction(); instr) lines.emplace_back(instr);
    else if (auto dot = dot_command(); dot) lines.emplace_back(dot);
    else if (auto comment = _buffer->match<tc::lex::InlineComment>(); comment) {
      auto line = std::make_shared<ir::CommentLine>(ir::attr::Comment(comment->pool, comment->id));
      line->source_interval = comment->location();
      lines.emplace_back(line);
    } else if (auto empty = _buffer->match<tc::lex::Empty>(); empty) {
      auto line = std::make_shared<ir::EmptyLine>();
      line->source_interval = empty->location();
      lines.emplace_back(line);
    } else {
      auto next = _buffer->peek();
      // TODO: post an error to the diag table
      throw std::logic_error("Unrecognized token: " + next->repr().toStdString());
    }
  }
  return lines;
}

std::shared_ptr<symbol::Table> pepp::tc::parser::PepParser::symbol_table() const { return _symtab; }

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::argument() {
  lex::Checkpoint cp(*_buffer);
  static constexpr auto le = bits::Order::LittleEndian;
  if (auto maybeInteger = _buffer->match<lex::Integer>(); maybeInteger) {
    if (maybeInteger->format == lex::Integer::Format::SignedDec)
      return std::make_shared<pas::ast::value::SignedDecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pas::ast::value::Hexadecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pas::ast::value::UnsignedDecimal>(maybeInteger->value, 2);
    else throw std::logic_error("Unrecognized integer format");
  } else if (auto maybeIdent = _buffer->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pas::ast::value::Symbolic>(entry);
  } else if (auto maybeChar = _buffer->match<lex::CharacterConstant>(); maybeChar) {
    return std::make_shared<pas::ast::value::Character>(maybeChar->value);
  } else if (auto maybeStr = _buffer->match<lex::StringConstant>(); maybeStr) {
    auto asStr = maybeStr->view().toString();
    if (asStr.size() <= 2) return std::make_shared<pas::ast::value::ShortString>(asStr, asStr.size(), le);
    return std::make_shared<pas::ast::value::LongString>(asStr, le);
  } else return nullptr;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::instruction() {
  using ISA = isa::Pep10;
  std::shared_ptr<pepp::tc::ir::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*_buffer);
  auto symbol = _buffer->match<lex::Identifier>();
  if (symbol) // TODO: check symbol length <7, else "Symbol too long"
    ;

  if (auto instruction = _buffer->match<lex::Identifier>(); !instruction) return cp.rollback(), nullptr;
  else if (auto instr = ISA::parseMnemonic(instruction->to_string()); instr == ISA::Mnemonic::INVALID)
    return /*cp.sync(),*/ nullptr;        // "Invalid Mnemonic"
  else if (ISA::isMnemonicUnary(instr)) { // Monadic instruction
    ret = std::make_shared<ir::MonadicInstruction>(ir::attr::Pep10Mnemonic(instr));
  } else { // Dyadic instruction
    ISA::AddressingMode am = ISA::AddressingMode::INVALID;
    auto arg = argument();
    if (!arg) return /*cp.sync(),*/ nullptr; // "Invalid argument"
    // TODO: check that argument fits in 2 bytes.
    // TODO: reject string arguments
    else if (_buffer->match_literal(",")) {
      auto addr_mode = _buffer->match<lex::Identifier>();
      // TODO: definitely a syntax error. "Expected addressing mode"
      if (!addr_mode) return /*cp.sync(),*/ nullptr;
      ISA::AddressingMode am = ISA::parseAddressingMode(addr_mode->to_string().toUpper());
      if (am == ISA::AddressingMode::INVALID) return /*cp.sync(),*/ nullptr; // "Invalid addressing mode"
      if (!ISA::isValidAddressingMode(instr, am))
        return /*cp.sync(),*/ nullptr; // "Illegal addressing mode for instruction"
    } else if (!ISA::requiresAddressingMode(instr)) am = ISA::defaultAddressingMode(instr);
    else return /*cp.sync(),*/ nullptr; // "Expected addressing mode"
    auto ir_instr = ir::attr::Pep10Mnemonic(instr);
    auto ir_addr = ir::attr::Pep10AddrMode(am);
    auto ir_arg = ir::attr::Argument(arg);
    ret = std::make_shared<ir::DyadicInstruction>(ir_instr, ir_addr, ir_arg);
  }
  if (symbol) ret->insert(std::make_unique<ir::attr::SymbolDeclaration>(_symtab->define(symbol->to_string())));
  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<ir::attr::Comment>(comment->pool, comment->id));

  return ret;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::dot_command() { return nullptr; }
