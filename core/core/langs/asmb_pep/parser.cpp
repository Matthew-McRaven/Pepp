#include "core/langs/asmb_pep/parser.hpp"
#include "core/arch/pep/isa/pep10.hpp"
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/lexer.hpp"
#include "core/langs/asmb_pep/parser_error.hpp"
#include "core/math/bitmanip/strings.hpp"

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data)
    : _pool(std::make_shared<std::unordered_set<std::string>>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(std::make_shared<pepp::core::symbol::LeafTable>(2)) {}

pepp::tc::IRProgram pepp::tc::parser::PepParser::parse(DiagnosticTable &diag) {
  IRProgram lines;
  while (_buffer->input_remains()) {
    try {
      if (auto line = statement(); line) lines.emplace_back(line);
    } catch (PepParserError &e) {
      synchronize();
      diag.add_message(e.loc, e.what());
    }
  }
  return lines;
}

std::shared_ptr<pepp::core::symbol::LeafTable> pepp::tc::parser::PepParser::symbol_table() const { return _symtab; }

void pepp::tc::parser::PepParser::debug_print_tokens(bool debug) { _lexer->print_tokens = debug; }

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::argument() {
  lex::Checkpoint cp(*_buffer);
  if (auto maybeInteger = _buffer->match<lex::Integer>(); maybeInteger) {
    if (maybeInteger->format == lex::Integer::Format::SignedDec)
      return std::make_shared<pepp::ast::SignedDecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pepp::ast::Hexadecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pepp::ast::UnsignedDecimal>(maybeInteger->value, 2);
    else throw PepParserError(PepParserError::NullaryError::Argument_InvalidIntegerFormat, _buffer->matched_interval());
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

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::numeric_argument() {
  auto arg = argument();
  return std::dynamic_pointer_cast<pepp::ast::Numeric>(arg);
}

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::hex_argument() {
  auto arg = argument();
  return std::dynamic_pointer_cast<pepp::ast::Hexadecimal>(arg);
}

std::shared_ptr<pepp::ast::Symbolic> pepp::tc::parser::PepParser::identifier_argument() {
  lex::Checkpoint cp(*_buffer);
  if (auto maybeIdent = _buffer->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pepp::ast::Symbolic>(2, entry);
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::instruction() {
  using ISA = isa::Pep10;
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*_buffer);

  if (auto instruction = _buffer->match<lex::Identifier>(); !instruction) return cp.rollback(), nullptr;
  else if (auto instr = ISA::parseMnemonic(instruction->to_string()); instr == ISA::Mnemonic::INVALID) {
    throw PepParserError(PepParserError::UnaryError::Mnemonic_Invalid, instruction->to_string(),
                         _buffer->matched_interval());
  } else if (ISA::isMnemonicUnary(instr)) { // Monadic instruction
    ret = std::make_shared<MonadicInstruction>(Pep10Mnemonic(instr));
  } else { // Dyadic instruction
    ISA::AddressingMode am = ISA::AddressingMode::INVALID;
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, _buffer->matched_interval());
    else if (arg->minimum_size() > 2)
      throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    // TODO: reject string arguments
    else if (_buffer->match_literal(",")) {
      auto addr_mode = _buffer->match<lex::Identifier>();
      if (!addr_mode)
        throw PepParserError(PepParserError::NullaryError::AddressingMode_Missing, _buffer->matched_interval());
      auto addr_mode_str = bits::to_upper(addr_mode->to_string());
      am = ISA::parseAddressingMode(addr_mode_str);
      if (am == ISA::AddressingMode::INVALID)
        throw PepParserError(PepParserError::NullaryError::AddressingMode_Invalid, _buffer->matched_interval());
      if (!ISA::isValidAddressingMode(instr, am))
        throw PepParserError(PepParserError::UnaryError::AddressingMode_InvalidForMnemonic, addr_mode_str,
                             _buffer->matched_interval());
    } else if (!ISA::requiresAddressingMode(instr)) am = ISA::defaultAddressingMode(instr);
    else throw PepParserError(PepParserError::NullaryError::AddressingMode_Required, _buffer->matched_interval());
    auto ir_instr = Pep10Mnemonic(instr);
    auto ir_addr = Pep10AddrMode(am);
    auto ir_arg = Argument(arg);
    ret = std::make_shared<DyadicInstruction>(ir_instr, ir_addr, ir_arg);
  }

  return ret;
}

namespace {
using DC = pepp::tc::DotCommands;
using PDC = pepp::tc::PepDotCommands;
static const auto dot_map =
    std::map<std::string, int>{{"ALIGN", (int)DC::ALIGN},    {"ASCII", (int)DC::ASCII},   {"BLOCK", (int)DC::BLOCK},
                               {"BYTE", (int)DC::BYTE},      {"EQUATE", (int)DC::EQUATE}, {"EXPORT", (int)PDC::EXPORT},
                               {"IMPORT", (int)PDC::IMPORT}, {"INPUT", (int)PDC::INPUT},  {"ORG", (int)DC::ORG},
                               {"OUTPUT", (int)PDC::OUTPUT}, {"SCALL", (int)PDC::SCALL},  {"SECTION", (int)DC::SECTION},
                               {"WORD", (int)DC::WORD}};
} // namespace
std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::pseudo(OptionalSymbol symbol) {
  auto dot = _buffer->match<lex::DotCommand>();
  auto dot_str = bits::to_upper(dot->to_string());
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend())
    throw PepParserError(PepParserError::UnaryError::Dot_Invalid, dot_str, _buffer->matched_interval());

  switch (it->second) {
  case (int)DC::ALIGN: {
    auto arg = numeric_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    u16 value;
    bits::span<u8> buf{(u8 *)&value, 2};
    (void)arg->serialize(buf, bits::hostOrder());
    if (!(value == 1 || value == 2 || value == 4 || value == 8))
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedPowerOfTwo, _buffer->matched_interval());
    return std::make_shared<DotAlign>(Argument{arg});
  }
  case (int)DC::ASCII: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()};
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)DC::BLOCK: {
    auto arg = numeric_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    return std::make_shared<DotBlock>(Argument{arg});
  }
  case (int)DC::BYTE: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 1)
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      if (ident->minimum_size() > 1)
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{ident});
    } else throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::EQUATE: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    else if (arg->minimum_size() > 2)
      throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    else if (!symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Required, _buffer->matched_interval());
    return std::make_shared<DotEquate>(SymbolDeclaration{*symbol}, Argument{arg});
  }
  case (int)PDC::EXPORT: {
    auto arg = identifier_argument();
    if (!arg)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    arg->symbol()->binding = pepp::core::symbol::Binding::Global;
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::EXPORT, Argument{arg});
  }
  case (int)PDC::IMPORT: {
    auto arg = identifier_argument();
    if (!arg)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    arg->symbol()->binding = pepp::core::symbol::Binding::Weak;
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::IMPORT, Argument{arg});
  }
  case (int)PDC::INPUT: {
    auto arg = identifier_argument();
    if (!arg)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::INPUT, Argument{arg});
  }
  case (int)DC::ORG: {
    auto arg = hex_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedHex, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotOrg>(DotOrg::Behavior::ORG, Argument{arg});
  }
  case (int)PDC::OUTPUT: {
    auto arg = identifier_argument();
    if (!arg)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::OUTPUT, Argument{arg});
  }
  case (int)PDC::SCALL: {
    auto arg = identifier_argument();
    if (!arg)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::SCALL, Argument{arg});
  }
  case (int)DC::SECTION: {
    if (auto maybeSecName = _buffer->match<lex::StringConstant>(); !maybeSecName)
      throw PepParserError(PepParserError::NullaryError::Section_StringName, _buffer->matched_interval());
    else if (!_buffer->match_literal(","))
      throw PepParserError(PepParserError::NullaryError::Section_TwoArgs, _buffer->matched_interval());
    else if (auto maybeFlags = _buffer->match<lex::StringConstant>(); !maybeFlags)
      throw PepParserError(PepParserError::NullaryError::Section_StringFlags, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    else {
      auto flags = bits::to_lower(maybeFlags->view());
      using bits::contains;
      bool r = contains(flags, "r"), w = contains(flags, "w"), x = contains(flags, "x"), z = contains(flags, "z");
      return std::make_shared<DotSection>(Identifier(*maybeSecName->value), SectionFlags(r, w, x, z));
    }
  }
  case (int)DC::WORD: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 2)
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{ident});
    } else throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::line(OptionalSymbol symbol) {
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto instr = instruction(); instr) ret = instr;
  else if (auto dot = pseudo(symbol); dot) ret = dot;
  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::statement() {
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
    if (symbol && symbol->to_string().length() > 7)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_TooLong, _buffer->matched_interval());

    auto symbol_decl = symbol ? OptionalSymbol(_symtab->define(symbol->to_string())) : std::nullopt;
    ret = line(symbol_decl);
    if (!ret) {
      auto next = _buffer->peek();
      throw PepParserError(PepParserError::UnaryError::Token_Invalid, next->repr(), _buffer->matched_interval());
    } else {
      ret->source_interval = _buffer->matched_interval();
    }
  }

  if (!_buffer->match<tc::lex::Empty>() && _buffer->input_remains())
    throw PepParserError(PepParserError::NullaryError::Token_MissingNewline, _buffer->matched_interval());
  return ret;
}

void pepp::tc::parser::PepParser::synchronize() {
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (_buffer->input_remains() && _buffer->match(mask));
}
