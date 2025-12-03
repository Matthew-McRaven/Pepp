#include "./pep_parser.hpp"
#include "./pep_attributes.hpp"
#include "./pep_tokens.hpp"
#include "fmt/format.h"
#include "toolchain/pas/ast/value/character.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/ast/value/hexadecimal.hpp"
#include "toolchain/pas/ast/value/numeric.hpp"
#include "toolchain/pas/ast/value/string.hpp"

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data)
    : _pool(std::make_shared<pepp::tc::support::StringPool>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(QSharedPointer<symbol::Table>::create(2)) {}

std::vector<std::shared_ptr<pepp::tc::ir::LinearIR>> pepp::tc::parser::PepParser::parse() {
  std::vector<std::shared_ptr<ir::LinearIR>> lines;

  while (_buffer->input_remains()) {
    if (auto line = statement(); line) lines.emplace_back(line);
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

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::numeric_argument() {
  auto arg = argument();
  if (!arg->isNumeric()) {
    throw std::logic_error("Expected a numeric argument");
  }
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
    synchronize();
    auto formatted = fmt::format("Invalid mnemonic \"{}\"", instruction->to_string().toStdString());
    throw std::logic_error(formatted);
  } else if (ISA::isMnemonicUnary(instr)) { // Monadic instruction
    ret = std::make_shared<ir::MonadicInstruction>(ir::attr::Pep10Mnemonic(instr));
  } else { // Dyadic instruction
    ISA::AddressingMode am = ISA::AddressingMode::INVALID;
    auto arg = argument();
    if (!arg) {
      synchronize();
      throw std::logic_error("Invalid argument");
    } else if (arg->requiredBytes() > 2) {
      synchronize();
      throw std::logic_error("Argument must fit in two bytes");
    }
    // TODO: reject string arguments
    else if (_buffer->match_literal(",")) {
      auto addr_mode = _buffer->match<lex::Identifier>();
      if (!addr_mode) {
        synchronize();
        throw std::logic_error("Expected addressing mode");
      }
      am = ISA::parseAddressingMode(addr_mode->to_string().toUpper());
      if (am == ISA::AddressingMode::INVALID) {
        synchronize();
        throw std::logic_error("Invalid addressing mode");
      }
      if (!ISA::isValidAddressingMode(instr, am)) {
        synchronize();
        auto formatted = fmt::format("Illegal addressing mode \"{}\"for instruction",
                                     addr_mode->to_string().toUpper().toStdString());
        throw std::logic_error(formatted);
      }
    } else if (!ISA::requiresAddressingMode(instr)) am = ISA::defaultAddressingMode(instr);
    else {
      synchronize();
      throw std::logic_error("Instruction requires addressing mode");
    }
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
std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::pseudo() {
  auto dot = _buffer->match<lex::DotCommand>();
  auto dot_str = dot->to_string().toUpper().toStdString();
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend()) {
    synchronize();
    auto formatted = fmt::format("Invalid pseudo-operation \"{}\"", dot_str);
    throw std::logic_error(formatted);
  }
  switch (it->second) {
  case ir::DotCommands::ALIGN: {
    auto arg = numeric_argument();
    quint16 value;
    bits::span<quint8> buf{(quint8 *)&value, 2};
    arg->value(buf, bits::hostOrder());
    if (value == 1 || value == 2 || value == 4 || value == 8) return std::make_shared<ir::DotAlign>(arg);
    else {
      synchronize();
      throw std::logic_error(".ALIGN argument must be (1|2|4|8)");
    }
  }

  case ir::DotCommands::ASCII: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr) {
      synchronize();
      throw std::logic_error(".ASCII requires a string argument");
    } else {
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
    if (arg->requiredBytes() > 1) {
      synchronize();
      throw std::logic_error(".BYTE argument must fit in one byte");
    }
    return std::make_shared<ir::DotLiteral>(ir::DotLiteral::Which::Byte, arg);
  }
  case ir::DotCommands::EQUATE: {
    auto arg = argument();
    if (arg->requiredBytes() > 2) {
      synchronize();
      throw std::logic_error(".EQUATE argument must fit in two bytes");
    }
    return std::make_shared<ir::DotEquate>(arg);
  }
  case ir::DotCommands::EXPORT: {
    auto arg = identifier_argument();
    if (!arg) {
      synchronize();
      throw std::logic_error(".OUTPUT requires an identifier argument");
    }
    return std::make_shared<ir::DotImportExport>(ir::DotImportExport::Direction::EXPORT, arg);
  }
  case ir::DotCommands::IMPORT: {
    auto arg = identifier_argument();
    if (!arg) {
      synchronize();
      throw std::logic_error(".OUTPUT requires an identifier argument");
    }
    return std::make_shared<ir::DotImportExport>(ir::DotImportExport::Direction::IMPORT, arg);
  }
  case ir::DotCommands::INPUT: {
    auto arg = identifier_argument();
    if (!arg) {
      synchronize();
      throw std::logic_error(".OUTPUT requires an identifier argument");
    }
    return std::make_shared<ir::DotInputOutput>(ir::DotInputOutput::Direction::INPUT, arg);
  }
  case ir::DotCommands::ORG:
  case ir::DotCommands::OUTPUT: {
    auto arg = identifier_argument();
    if (!arg) {
      synchronize();
      throw std::logic_error(".OUTPUT requires an identifier argument");
    }
    return std::make_shared<ir::DotInputOutput>(ir::DotInputOutput::Direction::OUTPUT, arg);
  }
  case ir::DotCommands::SCALL: {
    auto arg = identifier_argument();
    if (!arg) {
      synchronize();
      throw std::logic_error(".SCALL requires an identifier argument");
    }
    return std::make_shared<ir::DotSCall>(arg);
  }
  case ir::DotCommands::SECTION: {
    if (auto maybeSecName = _buffer->match<lex::StringConstant>(); !maybeSecName) {
      synchronize();
      throw std::logic_error(".SECTION name must be a string");
    } else if (!_buffer->match_literal(",")) {
      synchronize();
      throw std::logic_error(".SECTION requires two arguments");
    } else if (auto maybeFlags = _buffer->match<lex::StringConstant>(); !maybeFlags) {
      synchronize();
      throw std::logic_error(".SECTION flags must be a string");
    } else {
      static constexpr auto le = bits::Order::LittleEndian;
      std::shared_ptr<pas::ast::value::Base> arg;
      auto flags = maybeFlags->view().toString().toLower();
      bool r = flags.contains("r"), w = flags.contains("w"), x = flags.contains("x");
      return std::make_shared<ir::DotSection>(ir::attr::Identifier(maybeSecName->pool, maybeSecName->id),
                                              ir::attr::SectionFlags(r, w, x));
    }
  }
  case ir::DotCommands::WORD: {
    auto arg = numeric_argument();
    if (arg->requiredBytes() > 2) {
      synchronize();
      throw std::logic_error(".WORD argument must fit in two bytes");
    }
    return std::make_shared<ir::DotLiteral>(ir::DotLiteral::Which::Word, arg);
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::ir::LinearIR> pepp::tc::parser::PepParser::line() {
  std::shared_ptr<pepp::tc::ir::LinearIR> ret = nullptr;
  if (auto instr = instruction(); instr) ret = instr;
  else if (auto dot = pseudo(); dot) ret = dot;
  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<ir::attr::Comment>(comment->pool, comment->id));
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
    if (symbol && symbol->to_string().length() > 7) {
      auto formatted = fmt::format("Symbol \"{}\" too long", symbol->to_string().toStdString());
      throw std::logic_error(formatted);
    }
    ret = line();
    if (!ret) {
      auto next = _buffer->peek();
      synchronize();
      // TODO: post an error to the diag table
      throw std::logic_error("Unrecognized token: " + next->repr().toStdString());
    }
    if (symbol) ret->insert(std::make_unique<ir::attr::SymbolDeclaration>(_symtab->define(symbol->to_string())));
  }

  if (!_buffer->match<tc::lex::Empty>() && _buffer->input_remains()) {
    synchronize();
    throw std::logic_error("Expected \\n");
  }
  return ret;
}

void pepp::tc::parser::PepParser::synchronize() {
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (_buffer->input_remains() && _buffer->match(mask));
}
