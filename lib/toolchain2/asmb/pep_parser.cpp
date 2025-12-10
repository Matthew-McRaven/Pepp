#include "./pep_parser.hpp"
#include "./pep_attributes.hpp"
#include "./pep_tokens.hpp"
#include "common_diag.hpp"
#include "toolchain/pas/ast/value/character.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/ast/value/hexadecimal.hpp"
#include "toolchain/pas/ast/value/string.hpp"

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data)
    : _pool(std::make_shared<pepp::tc::support::StringPool>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(QSharedPointer<symbol::Table>::create(2)) {}

pepp::tc::PepIRProgram pepp::tc::parser::PepParser::parse(DiagnosticTable &diag) {
  PepIRProgram lines;
  while (_buffer->input_remains()) {
    try {
      if (auto line = statement(); line) lines.emplace_back(line);
    } catch (ParserError &e) {
      synchronize();
      diag.add_message(e.loc, e.what());
    }
  }
  return lines;
}

QSharedPointer<symbol::Table> pepp::tc::parser::PepParser::symbol_table() const { return _symtab; }

void pepp::tc::parser::PepParser::debug_print_tokens(bool debug) { _lexer->print_tokens = debug; }

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
    else throw ParserError(ParserError::NullaryError::Argument_InvalidIntegerFormat, _buffer->matched_interval());
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

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::numeric_argument() {
  auto arg = argument();
  if (!arg->isNumeric()) return nullptr;
  return arg;
}

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::hex_argument() {
  auto arg = argument();
  if (auto as_hex = std::dynamic_pointer_cast<pas::ast::value::Hexadecimal>(arg); !as_hex) return nullptr;
  return arg;
}

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::identifier_argument() {
  lex::Checkpoint cp(*_buffer);
  static constexpr auto le = bits::Order::LittleEndian;
  if (auto maybeIdent = _buffer->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pas::ast::value::Symbolic>(entry);
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::instruction() {
  using ISA = isa::Pep10;
  std::shared_ptr<pepp::tc::ir::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*_buffer);

  if (auto instruction = _buffer->match<lex::Identifier>(); !instruction) return cp.rollback(), nullptr;
  else if (auto instr = ISA::parseMnemonic(instruction->to_string()); instr == ISA::Mnemonic::INVALID) {
    throw ParserError(ParserError::UnaryError::Mnemonic_Invalid, instruction->to_string().toStdString(),
                      _buffer->matched_interval());
  } else if (ISA::isMnemonicUnary(instr)) { // Monadic instruction
    ret = std::make_shared<ir::MonadicInstruction>(ir::attr::Pep10Mnemonic(instr));
  } else { // Dyadic instruction
    ISA::AddressingMode am = ISA::AddressingMode::INVALID;
    auto arg = argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_Missing, _buffer->matched_interval());
    else if (arg->requiredBytes() > 2)
      throw ParserError(ParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    // TODO: reject string arguments
    else if (_buffer->match_literal(",")) {
      auto addr_mode = _buffer->match<lex::Identifier>();
      if (!addr_mode) throw ParserError(ParserError::NullaryError::AddressingMode_Missing, _buffer->matched_interval());
      auto addr_mode_str = addr_mode->to_string().toUpper();
      am = ISA::parseAddressingMode(addr_mode_str);
      if (am == ISA::AddressingMode::INVALID)
        throw ParserError(ParserError::NullaryError::AddressingMode_Invalid, _buffer->matched_interval());
      if (!ISA::isValidAddressingMode(instr, am))
        throw ParserError(ParserError::UnaryError::AddressingMode_InvalidForMnemonic, addr_mode_str.toStdString(),
                          _buffer->matched_interval());
    } else if (!ISA::requiresAddressingMode(instr)) am = ISA::defaultAddressingMode(instr);
    else throw ParserError(ParserError::NullaryError::AddressingMode_Required, _buffer->matched_interval());
    auto ir_instr = ir::attr::Pep10Mnemonic(instr);
    auto ir_addr = ir::attr::Pep10AddrMode(am);
    auto ir_arg = ir::attr::Argument(arg);
    ret = std::make_shared<ir::DyadicInstruction>(ir_instr, ir_addr, ir_arg);
  }

  return ret;
}

namespace {
using DC = pepp::tc::ir::DotCommands;
static const auto dot_map = std::map<std::string, DC>{
    {"ALIGN", DC::ALIGN},   {"ASCII", DC::ASCII},     {"BLOCK", DC::BLOCK}, {"BYTE", DC::BYTE}, {"EQUATE", DC::EQUATE},
    {"EXPORT", DC::EXPORT}, {"IMPORT", DC::IMPORT},   {"INPUT", DC::INPUT}, {"ORG", DC::ORG},   {"OUTPUT", DC::OUTPUT},
    {"SCALL", DC::SCALL},   {"SECTION", DC::SECTION}, {"WORD", DC::WORD}};
} // namespace
std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::pseudo(OptionalSymbol symbol) {
  auto dot = _buffer->match<lex::DotCommand>();
  auto dot_str = dot->to_string().toUpper().toStdString();
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend())
    throw ParserError(ParserError::UnaryError::Dot_Invalid, dot_str, _buffer->matched_interval());

  switch (it->second) {
  case ir::DotCommands::ALIGN: {
    auto arg = numeric_argument();
    quint16 value;
    bits::span<quint8> buf{(quint8 *)&value, 2};
    arg->value(buf, bits::hostOrder());
    if (!(value == 1 || value == 2 || value == 4 || value == 8))
      throw ParserError(ParserError::NullaryError::Argument_ExpectedPowerOfTwo, _buffer->matched_interval());
    return std::make_shared<ir::DotAlign>(arg);
  }
  case ir::DotCommands::ASCII: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr)
      throw ParserError(ParserError::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      static constexpr auto le = bits::Order::LittleEndian;
      auto asStr = maybeStr->view().toString();
      std::shared_ptr<pas::ast::value::Base> arg;
      if (asStr.size() <= 2) arg = std::make_shared<pas::ast::value::ShortString>(asStr, asStr.size(), le);
      else arg = std::make_shared<pas::ast::value::LongString>(asStr, le);

      return std::make_shared<ir::DotLiteral>(ir::DotLiteral::Which::ASCII, arg);
    }
  }
  case ir::DotCommands::BLOCK: {
    auto arg = numeric_argument();
    return std::make_shared<ir::DotBlock>(arg);
  }
  case ir::DotCommands::BYTE: {
    auto arg = numeric_argument();
    if (arg->requiredBytes() > 1)
      throw ParserError(ParserError::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
    return std::make_shared<ir::DotLiteral>(ir::DotLiteral::Which::Byte, arg);
  }
  case ir::DotCommands::EQUATE: {
    auto arg = argument();
    if (arg->requiredBytes() > 2)
      throw ParserError(ParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    else if (!symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Required, _buffer->matched_interval());
    QSharedPointer<symbol::Entry> symbol_entry = *symbol;
    return std::make_shared<ir::DotEquate>(ir::attr::SymbolDeclaration{symbol_entry}, arg);
  }
  case ir::DotCommands::EXPORT: {
    auto arg = identifier_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotAnnotate>(ir::DotAnnotate::Which::EXPORT, arg);
  }
  case ir::DotCommands::IMPORT: {
    auto arg = identifier_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotAnnotate>(ir::DotAnnotate::Which::IMPORT, arg);
  }
  case ir::DotCommands::INPUT: {
    auto arg = identifier_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotAnnotate>(ir::DotAnnotate::Which::INPUT, arg);
  }
  case ir::DotCommands::ORG: {
    auto arg = hex_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedHex, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotOrg>(ir::DotOrg::Behavior::ORG, arg);
  }
  case ir::DotCommands::OUTPUT: {
    auto arg = identifier_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotAnnotate>(ir::DotAnnotate::Which::OUTPUT, arg);
  }
  case ir::DotCommands::SCALL: {
    auto arg = identifier_argument();
    if (!arg) throw ParserError(ParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<ir::DotAnnotate>(ir::DotAnnotate::Which::SCALL, arg);
  }
  case ir::DotCommands::SECTION: {
    if (auto maybeSecName = _buffer->match<lex::StringConstant>(); !maybeSecName)
      throw ParserError(ParserError::NullaryError::Section_StringName, _buffer->matched_interval());
    else if (!_buffer->match_literal(","))
      throw ParserError(ParserError::NullaryError::Section_TwoArgs, _buffer->matched_interval());
    else if (auto maybeFlags = _buffer->match<lex::StringConstant>(); !maybeFlags)
      throw ParserError(ParserError::NullaryError::Section_StringFlags, _buffer->matched_interval());
    else if (symbol)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    else {
      static constexpr auto le = bits::Order::LittleEndian;
      std::shared_ptr<pas::ast::value::Base> arg;
      auto flags = maybeFlags->view().toString().toLower();
      bool r = flags.contains("r"), w = flags.contains("w"), x = flags.contains("x"), z = flags.contains("z");
      return std::make_shared<ir::DotSection>(ir::attr::Identifier(maybeSecName->pool, maybeSecName->id),
                                              ir::attr::SectionFlags(r, w, x, z));
    }
  }
  case ir::DotCommands::WORD: {
    auto arg = numeric_argument();
    if (arg->requiredBytes() > 2)
      throw ParserError(ParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    return std::make_shared<ir::DotLiteral>(ir::DotLiteral::Which::Word, arg);
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::line(OptionalSymbol symbol) {
  std::shared_ptr<pepp::tc::ir::LinearIR> ret = nullptr;
  if (auto instr = instruction(); instr) ret = instr;
  else if (auto dot = pseudo(symbol); dot) ret = dot;
  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<ir::attr::Comment>(comment->pool, comment->id));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<ir::attr::SymbolDeclaration>())
    ret->insert(std::make_unique<ir::attr::SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::statement() {
  std::shared_ptr<pepp::tc::ir::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*_buffer);

  if (auto empty = _buffer->match<tc::lex::Empty>(); empty) {
    auto line = std::make_shared<ir::EmptyLine>();
    line->source_interval = empty->location();
    return line;
  }

  if (auto comment = _buffer->match<tc::lex::InlineComment>(); comment) {
    auto line = std::make_shared<ir::CommentLine>(ir::attr::Comment(comment->pool, comment->id));
    line->source_interval = comment->location();
    ret = line;
  } else {
    auto symbol = _buffer->match<lex::SymbolDeclaration>();
    if (symbol && symbol->to_string().length() > 7)
      throw ParserError(ParserError::NullaryError::SymbolDeclaration_TooLong, _buffer->matched_interval());

    auto symbol_decl = symbol ? OptionalSymbol(_symtab->define(symbol->to_string())) : std::nullopt;
    ret = line(symbol_decl);
    if (!ret) {
      auto next = _buffer->peek();
      throw ParserError(ParserError::UnaryError::Token_Invalid, next->repr().toStdString(),
                        _buffer->matched_interval());
    } else {
      ret->source_interval = _buffer->matched_interval();
    }
  }

  if (!_buffer->match<tc::lex::Empty>() && _buffer->input_remains())
    throw ParserError(ParserError::NullaryError::Token_MissingNewline, _buffer->matched_interval());
  return ret;
}

void pepp::tc::parser::PepParser::synchronize() {
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (_buffer->input_remains() && _buffer->match(mask));
}
