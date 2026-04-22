#include "core/langs/asmb_pep/parser.hpp"
#include "core/arch/pep/isa/pep10.hpp"
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
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
#include "core/langs/asmb_pep/text_format.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "spdlog/spdlog.h"

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data, std::shared_ptr<MacroRegistry> reg)
    : _pool(std::make_shared<std::unordered_set<std::string>>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(std::make_shared<pepp::core::symbol::LeafTable>(2)), _macros(reg) {}

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

static const auto split_args = [](std::shared_ptr<pepp::tc::lex::Token> const &t) {
  if (t->type() != pepp::tc::lex::Literal::TYPE) return false;
  auto lit = std::static_pointer_cast<pepp::tc::lex::Literal>(t);
  return lit->literal == ",";
};

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::macro(OptionalSymbol symbol) {
  // helper predicate to split token span on comma literals.

  lex::Checkpoint cp(*_buffer);
  auto maybe_macro = _buffer->match<lex::Identifier>();
  if (!maybe_macro) return cp.rollback(), nullptr;
  auto macro = maybe_macro->to_string();
  auto macro_def = _macros->find(macro);
  if (macro_def == nullptr) return cp.rollback(), nullptr;
  else cp.commit();

  while (auto matched = _buffer->match_not<tc::lex::Empty, tc::lex::EoF, tc::lex::InlineComment>()) {
    // Consume all no-comments, non-empty tokens until the end of the current line.
  }
  // Get tokens after the macro name, split on commas, re-assmble to strings.
  auto tokens = _buffer->matched_tokens_after(cp.marker());
  std::vector<std::string> args;
  std::span<std::shared_ptr<pepp::tc::lex::Token> const> head, rest = tokens;
  while (!rest.empty()) {
    std::tie(head, rest) = pepp::tc::split_exclusive(rest, split_args);
    auto arg = token_join(head);
    SPDLOG_WARN("Parsed macro argument: '{}'", arg);
    args.emplace_back(arg);
  }
  SPDLOG_WARN("Parsed macro invocation: '{}', with {} arguments", macro, args.size());
  auto ret = std::make_shared<MacroInstantiation>(macro_def, args);
  // TODO: Validate # of matched arguments vs number of args in definition, accounting for default values.
  // TODO: Attach symbol def
  // TODO: push an entry on the conditional stack to enter skip mode.
  return ret;
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
static const auto dot_map = std::map<std::string, int>{
    {"ALIGN", (int)DC::ALIGN},    {"ASCII", (int)DC::ASCII},   {"BLOCK", (int)DC::BLOCK},
    {"BYTE", (int)DC::BYTE},      {"EQUATE", (int)DC::EQUATE}, {"EXPORT", (int)PDC::EXPORT},
    {"IMPORT", (int)PDC::IMPORT}, {"INPUT", (int)PDC::INPUT},  {"ORG", (int)DC::ORG},
    {"OUTPUT", (int)PDC::OUTPUT}, {"SCALL", (int)PDC::SCALL},  {"SECTION", (int)DC::SECTION},
    {"WORD", (int)DC::WORD},      {"IF", (int)DC::IF},         {"ELSEIF", (int)DC::ELSEIF},
    {"ELSE", (int)DC::ELSE},      {"ENDIF", (int)DC::ENDIF},   {"MACRO", (int)DC::INLINE_MACRO}};
} // namespace
std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::pseudo(OptionalSymbol symbol) {
  auto dot = _buffer->match<lex::DotCommand>();
  if (!dot) return nullptr;
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
    return std::make_shared<DotAlign>(DotAlign::Which::ByteCount, Argument{arg});
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
  case (int)DC::IF: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    bool matched = arg->value_as<i16>() != 0;
    _conditionals.emplace_back(
        ConditionalStack{.matched_any = matched, .matched_this_stmt = matched, .matched_else = false});
    return std::make_shared<DotConditional>(DotConditional::Behavior::IF, Argument{arg});
  }
  case (int)DC::ELSEIF: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, _buffer->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    else if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElseif, _buffer->matched_interval());

    auto &tos = _conditionals.back();
    if (tos.matched_else) {
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElseif, _buffer->matched_interval());
    } else if (tos.matched_any) _conditionals.back().matched_this_stmt = false;
    else {
      tos.matched_this_stmt = (arg->value_as<i16>() != 0);
      tos.matched_any = tos.matched_any || tos.matched_this_stmt;
    }
    return std::make_shared<DotConditional>(DotConditional::Behavior::ELSEIF, Argument{arg});
  }
  case (int)DC::ELSE: {
    if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    else if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElse, _buffer->matched_interval());

    auto &tos = _conditionals.back();
    if (tos.matched_else)
      throw PepParserError(PepParserError::NullaryError::Conditional_MultipleElse, _buffer->matched_interval());
    else if (tos.matched_any) tos.matched_this_stmt = false;
    else {
      tos.matched_this_stmt = true;
      tos.matched_any = true;
    }
    tos.matched_else = true;
    return std::make_shared<DotConditional>(DotConditional::Behavior::ELSE);
  }
  case (int)DC::ENDIF: {
    if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedEndif, _buffer->matched_interval());
    _conditionals.pop_back();
    return std::make_shared<DotConditional>(DotConditional::Behavior::ENDIF);
  }
  case (int)DC::INLINE_MACRO: {
    auto name = identifier_argument();
    if (!name)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    lex::Checkpoint cp(*_buffer);
    auto tokens = _buffer->matched_tokens_after(cp.marker());
    std::vector<std::string> args;
    std::span<std::shared_ptr<pepp::tc::lex::Token> const> head, rest = tokens;
    while (!rest.empty()) {
      std::tie(head, rest) = pepp::tc::split_exclusive(rest, split_args);
      auto arg = token_join(head);
      SPDLOG_WARN("Defining macro argument: '{}'", arg);
      args.emplace_back(arg);
    }
    SPDLOG_WARN("Defining inline macro: '{}', with {} arguments", name->string(), args.size());
    _active_macro_defs++;
    return std::make_shared<InlineMacroDefinition>(name->string(), args);
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::line(OptionalSymbol symbol) {
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto dot = pseudo(symbol); dot) ret = dot;
  else if (auto macro = this->macro(symbol); macro) ret = macro;
  else if (auto instr = instruction(); instr) ret = instr;

  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::statement() {
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;

  {
    // Limit lifetime of checkpoint to avoid clearing pushed-back token after skip loop.
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
    }
    // Avoid line() if no input remains (e.g., due to unterminated macro or conditional).
    else if (_buffer->input_remains()) {
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
  }
  // Start skip loop _after_ parsing the statement which entered the skip loop.
  // This way we can preserve an invariant that the first token consumed by statement is a part of the returned IR line.
  // This is particularly helpful for macros definitions where we need to associate the macro IR object with its inline
  // body.
  auto start_depth = _conditionals.size();
  const auto start_ival = _lexer->current_location();
  // Consume tokens directly from lexer without buffering to avoid buffer-clearing bugs.
  while ((_active_macro_defs > 0 || skip_mode()) && _lexer->input_remains()) {
    auto token = _lexer->next_token();
    if (_active_macro_defs > 0) {
      // Need to count start / ends of macro definitions.
      if (token && token->type() == lex::DotCommand::TYPE) {
        auto dot_str = bits::to_upper(token->to_string());
        if (dot_str == "MACRO") _active_macro_defs++;
        else if (dot_str == "ENDM") _active_macro_defs--;
      }
      if (_active_macro_defs == 0) {
        // TODO: emplace the macro definition in the registry using the re-assembled/collected body tokens.
      }
    }
    // Check if the next line is a conditional directive that could increase our skip depth.
    // Do not consume the ENDIF which leaves skip mode so that we can properly emit the IR line.
    else if (token && token->type() == lex::DotCommand::TYPE) {
      auto dot_str = bits::to_upper(token->to_string());
      // If we hit an .IF, increment our conditional depth to avoid confusion with nested inactive conditionals.
      if (dot_str == "IF")
        _conditionals.emplace_back(
            ConditionalStack{.matched_any = false, .matched_this_stmt = false, .matched_else = false});
      else if ((dot_str == "ELSEIF" || dot_str == "ELSE") && start_depth == _conditionals.size() &&
               !_conditionals.back().matched_any) {
        // Need to parse this branch! It may make our condition true.
        _buffer->push_token(token);
        break;
      } else if (dot_str == "ENDIF") {
        if (start_depth < _conditionals.size()) _conditionals.pop_back();
        // Do not consume ENDIF token closing the conditional that entered skip mode. Re-buffer that token so we can
        // take a normal parsing path for it and emit the proper IR for the closing directive.
        else {
          _buffer->push_token(token);
          break;
        }
      }
    }
  }

  if (!_buffer->input_remains() && _active_macro_defs > 0) {
    const auto end_ival = _lexer->current_location();
    support::LocationInterval ival{start_ival, end_ival};
    throw PepParserError(PepParserError::NullaryError::Macro_Unterminated, ival);
  } else if (!_buffer->input_remains() && _conditionals.size() > 0) {
    const auto end_ival = _lexer->current_location();
    support::LocationInterval ival{start_ival, end_ival};
    throw PepParserError(PepParserError::NullaryError::Conditional_Unterminated, ival);
  }

  return ret;
}

void pepp::tc::parser::PepParser::synchronize() {
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (_buffer->input_remains() && _buffer->match(mask));
}

bool pepp::tc::parser::PepParser::skip_mode() const {
  return std::accumulate(_conditionals.begin(), _conditionals.end(), false,
                         [](bool acc, const ConditionalStack &cs) { return acc || (!cs.matched_this_stmt); });
}
