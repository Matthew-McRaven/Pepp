#include "core/langs/asmb_riscv/parser.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/parser_error.hpp"
#include "core/math/bitmanip/strings.hpp"

pepp::tc::parser::RISCVParser::RISCVParser(support::SeekableData &&data)
    : _pool(std::make_shared<std::unordered_set<std::string>>()),
      _lexer(std::make_shared<langs::RISCVLexer>(_pool, std::move(data))),
      _buffer(std::make_shared<lex::Buffer>(&*_lexer)), _symtab(std::make_shared<pepp::core::symbol::LeafTable>(2)) {}

pepp::tc::RISCVIRProgram pepp::tc::parser::RISCVParser::parse(DiagnosticTable &diag) {
  RISCVIRProgram lines;
  while (_buffer->input_remains()) {
    try {
      if (auto line = statement(); line) lines.emplace_back(line);
    } catch (RISCVParserError &e) {
      synchronize();
      diag.add_message(e.loc, e.what());
    }
  }
  return lines;
}

void pepp::tc::parser::RISCVParser::debug_print_tokens(bool debug) { _lexer->print_tokens = debug; }

std::optional<u8> pepp::tc::parser::RISCVParser::register_integer() {
  lex::Checkpoint cp(*_buffer);
  if (auto regs = _buffer->match<lex::Identifier>(); !regs) return cp.rollback(), std::nullopt;
  else if (auto reg_num = riscv::parse_register(regs->to_string()); reg_num.has_value()) return reg_num.value();
  else return cp.rollback(), std::nullopt;
}

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::RISCVParser::argument() {
  lex::Checkpoint cp(*_buffer);
  if (auto maybeInteger = _buffer->match<lex::Integer>(); maybeInteger) {
    if (maybeInteger->format == lex::Integer::Format::SignedDec)
      return std::make_shared<pepp::ast::SignedDecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pepp::ast::Hexadecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pepp::ast::UnsignedDecimal>(maybeInteger->value, 2);
    else
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_InvalidIntegerFormat,
                             _buffer->matched_interval());
  } else if (auto maybeIdent = _buffer->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pepp::ast::Symbolic>(2, entry);
  } else if (auto maybeChar = _buffer->match<lex::CharacterConstant>(); maybeChar) {
    return std::make_shared<pepp::ast::Character>(maybeChar->value[0]);
  } else if (auto maybeStr = _buffer->match<lex::StringConstant>(); maybeStr) {
    auto asStr = std::string{maybeStr->view()};
    return std::make_shared<pepp::ast::String>(asStr);
  } else return nullptr;
}

std::shared_ptr<pepp::tc::RTypeIR> pepp::tc::parser::RISCVParser::r_type(riscv::MnemonicDescriptor desc) {
  if (const auto rd = register_integer(); !rd)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRD, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (const auto rs1 = register_integer(); !rs1)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS1, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (const auto rs2 = register_integer(); !rs2)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS2, _buffer->matched_interval());
  else return std::make_shared<RTypeIR>(desc, rd.value(), rs1.value(), rs2.value());
}

std::shared_ptr<pepp::tc::ITypeIR> pepp::tc::parser::RISCVParser::i_type_load(riscv::MnemonicDescriptor desc) {
  if (const auto rd = register_integer(); !rd)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRD, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (auto arg = argument(); !arg)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  else if (!_buffer->match_literal("("))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingLParen, _buffer->matched_interval());
  else if (const auto rs = register_integer(); !rs)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  else if (!_buffer->match_literal(")"))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingRParen, _buffer->matched_interval());
  else return std::make_shared<ITypeIR>(desc, rd.value(), rs.value(), arg);
}

std::shared_ptr<pepp::tc::ITypeIR> pepp::tc::parser::RISCVParser::i_type_arith(riscv::MnemonicDescriptor desc) {
  if (const auto rd = register_integer(); !rd)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRD, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (const auto rs1 = register_integer(); !rs1)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS1, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (const auto imm = argument(); !imm)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedImm, _buffer->matched_interval());
  else return std::make_shared<ITypeIR>(desc, rd.value(), rs1.value(), imm);
}

std::shared_ptr<pepp::tc::STypeIR> pepp::tc::parser::RISCVParser::s_type(riscv::MnemonicDescriptor desc) {
  if (const auto rs2 = register_integer(); !rs2)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS2, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (auto arg = argument(); !arg)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  else if (!_buffer->match_literal("("))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingLParen, _buffer->matched_interval());
  else if (const auto rs1 = register_integer(); !rs1)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  else if (!_buffer->match_literal(")"))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingRParen, _buffer->matched_interval());
  else return std::make_shared<STypeIR>(desc, rs1.value(), rs2.value(), arg);
}

std::shared_ptr<pepp::tc::BTypeIR> pepp::tc::parser::RISCVParser::b_type(riscv::MnemonicDescriptor desc) {
  if (const auto rs1 = register_integer(); !rs1)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS1, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (const auto rs2 = register_integer(); !rs2)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedRS2, _buffer->matched_interval());
  else if (!_buffer->match_literal(","))
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingComma, _buffer->matched_interval());
  else if (auto arg = argument(); !arg)
    throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  else return std::make_shared<BTypeIR>(desc, rs1.value(), rs2.value(), arg);
}
std::shared_ptr<pepp::tc::IntegerInstruction> pepp::tc::parser::RISCVParser::instruction() {
  lex::Checkpoint cp(*_buffer);
  const auto maybe_instr = _buffer->match<lex::Identifier>();
  if (!maybe_instr) return cp.rollback(), nullptr;
  auto instr_str = maybe_instr->to_string();
  bits::to_lower_inplace(instr_str);
  const auto maybe_desc = riscv::string_to_mnemonic.find(instr_str);
  if (maybe_desc == riscv::string_to_mnemonic.end()) return cp.rollback(), nullptr;
  const auto desc = *maybe_desc;

  switch (desc.mn.type()) {
  case riscv::MnemonicDescriptor::Type::R: return r_type(desc.mn);
  case riscv::MnemonicDescriptor::Type::I:
    if (desc.mn.opcode() == RV32I_LOAD) return i_type_load(desc.mn);
    else return i_type_arith(desc.mn);
  case riscv::MnemonicDescriptor::Type::S: return s_type(desc.mn);
  case riscv::MnemonicDescriptor::Type::B: return b_type(desc.mn);
  case riscv::MnemonicDescriptor::Type::J: break;
  case riscv::MnemonicDescriptor::Type::U: break;
  case riscv::MnemonicDescriptor::Type::Pseudo: break;
  default: break;
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::RISCVParser::line(OptionalSymbol symbol) {
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto instr = instruction(); instr) ret = instr;
  // else if (auto dot = pseudo(symbol); dot) ret = dot;
  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::RISCVParser::statement() {
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*_buffer);

  if (auto empty = _buffer->match<tc::lex::Empty>(); empty) {
    auto line = std::make_shared<EmptyLine>();
    line->source_interval = empty->location();
    return line;
  }

  if (auto comment = _buffer->match<tc::lex::InlineComment>(); comment) {
    auto line = std::make_shared<CommentLine>(Comment(*comment->value));
    line->source_interval = comment->location();
    ret = line;
  } else {
    auto symbol = _buffer->match<lex::SymbolDeclaration>();
    auto symbol_decl = symbol ? OptionalSymbol(_symtab->define(symbol->to_string())) : std::nullopt;
    ret = line(symbol_decl);
    if (!ret) {
      auto next = _buffer->peek();
      throw RISCVParserError(RISCVParserError::UnaryError::Token_Invalid, next->repr(), _buffer->matched_interval());
    } else {
      ret->source_interval = _buffer->matched_interval();
    }
  }

  if (!_buffer->match<tc::lex::Empty>() && _buffer->input_remains())
    throw RISCVParserError(RISCVParserError::NullaryError::Token_MissingNewline, _buffer->matched_interval());
  return ret;
}

void pepp::tc::parser::RISCVParser::synchronize() {
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (_buffer->input_remains() && _buffer->match(mask));
}
