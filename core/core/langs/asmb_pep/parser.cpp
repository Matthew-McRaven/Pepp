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
#include "core/compile/macro/macro_replacement.hpp"
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
      _root_lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))),
      _symtab(std::make_shared<pepp::core::symbol::LeafTable>(2)), _macros(reg) {
  auto buffer = std::make_shared<lex::Buffer>(&*_root_lexer);
  _lexer_stack.emplace(_root_lexer, buffer);
}

pepp::tc::IRProgram pepp::tc::parser::PepParser::parse(DiagnosticTable &diag) { return do_parse(diag, std::nullopt); }

std::shared_ptr<pepp::core::symbol::LeafTable> pepp::tc::parser::PepParser::symbol_table() const { return _symtab; }

void pepp::tc::parser::PepParser::debug_print_tokens(bool debug) { _root_lexer->print_tokens = debug; }

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::argument() {
  auto buf = active_buffer();
  lex::Checkpoint cp(*buf);
  if (auto maybeInteger = buf->match<lex::Integer>(); maybeInteger) {
    if (maybeInteger->format == lex::Integer::Format::SignedDec)
      return std::make_shared<pepp::ast::SignedDecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pepp::ast::Hexadecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pepp::ast::UnsignedDecimal>(maybeInteger->value, 2);
    else throw PepParserError(PepParserError::NullaryError::Argument_InvalidIntegerFormat, buf->matched_interval());
  } else if (auto maybeIdent = buf->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pepp::ast::Symbolic>(2, entry);
  } else if (auto maybeChar = buf->match<lex::CharacterConstant>(); maybeChar) {
    return std::make_shared<pepp::ast::Character>(maybeChar->value[0]);
  } else if (auto maybeStr = buf->match<lex::StringConstant>(); maybeStr) {
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
  auto buf = active_buffer();
  lex::Checkpoint cp(*buf);
  if (auto maybeIdent = buf->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pepp::ast::Symbolic>(2, entry);
  }
  return nullptr;
}

static const u8 MAX_PARSE_DEPTH = 4;
pepp::tc::IRProgram pepp::tc::parser::PepParser::do_parse(DiagnosticTable &diag,
                                                          std::optional<support::LocationInterval> root_loc) {
  // Prevent infinite recursion of macro expansions by arbitrarily bounding parse depth.
  if (_lexer_stack.size() > MAX_PARSE_DEPTH) {
    // We've only entered the loop n-1 times, but we need to pop n lexers.
    // Pop an extra one here for that sake.
    _lexer_stack.pop();
    throw PepRecursionError(root_loc.value_or(support::LocationInterval()));
  }
  auto buf = active_buffer();
  IRProgram lines;
  while (buf->input_remains()) {
    try {
      if (auto line = statement(diag); line) {
        // if(root_loc) line->source_interval = *root_loc;
        lines.emplace_back(line);
      }
    } catch (PepRecursionError &e) {
      if (_lexer_stack.size() == 2) {
        // About to re-enter the root context, convert to a normal parser error so we can use the normal logging
        _lexer_stack.pop();
        throw PepParserError(PepParserError::NullaryError::Macro_ExcessiveRecursion, root_loc.value_or(e.loc));
      } else {
        // Bubble up error until we reach the root context.
        _lexer_stack.pop();
        throw;
      }
    } catch (PepParserError &e) {
      synchronize();
      diag.add_message(root_loc.value_or(e.loc), e.what());
    }
  }
  // Pop the lexer for this context before returning to caller.
  // Don't pop root context, we probably want it for later.
  if (_lexer_stack.size() > 1) _lexer_stack.pop();
  return lines;
}

static const auto split_args = [](std::shared_ptr<pepp::tc::lex::Token> const &t) {
  if (t->type() != pepp::tc::lex::Literal::TYPE) return false;
  auto lit = std::static_pointer_cast<pepp::tc::lex::Literal>(t);
  return lit->literal == ",";
};

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::macro(DiagnosticTable &diag, OptionalSymbol symbol) {
  auto buf = active_buffer();
  auto lexer = active_lexer();
  // helper predicate to split token span on comma literals.
  lex::Checkpoint cp(*buf);
  auto maybe_macro = buf->match<lex::Identifier>();
  if (!maybe_macro) return cp.rollback(), nullptr;
  auto macro = maybe_macro->to_string();
  auto macro_def = _macros->find(macro);
  if (macro_def == nullptr) return cp.rollback(), nullptr;
  else cp.commit();

  while (auto matched = buf->match_not<tc::lex::Empty, tc::lex::EoF, tc::lex::InlineComment>()) {
    // Consume all no-comments, non-empty tokens until the end of the current line.
  }
  // Get tokens after the macro name, split on commas, re-assmble to strings.
  auto tokens = buf->matched_tokens_after(cp.marker());
  std::vector<std::string> args;
  std::span<std::shared_ptr<pepp::tc::lex::Token> const> head, rest = tokens;
  while (!rest.empty()) {
    std::tie(head, rest) = pepp::tc::split_exclusive(rest, split_args);
    const auto first_loc = head.front()->location().lower(), last_loc = head.back()->location().upper();
    auto arg = lexer->view(support::LocationInterval(first_loc, last_loc));
    args.emplace_back(arg);
  }
  auto ret = std::make_shared<MacroInstantiation>(macro_def, args);
  auto rep = _counters.counters_for(macro_def->name);
  // TODO: Validate # of matched arguments vs number of args in definition, accounting for default values.
  for (int it = 0; it < macro_def->arguments.size(); it++) {
    const auto arg_name = macro_def->arguments.at(it).name;
    const auto arg_value = args.size() > it ? args.at(it) : macro_def->arguments.at(it).default_value.value_or("");
    rep["\\" + arg_name] = arg_value;
  }
  auto new_body = bits::rtrimmed(replace_macro_arguments(macro_def->body, rep));
  auto new_lexer = std::make_shared<lex::PepLexer>(_pool, support::SeekableData{std::move(new_body)});
  auto new_buffer = std::make_shared<lex::Buffer>(&*new_lexer);
  _lexer_stack.emplace(new_lexer, new_buffer);
  ret->lines = do_parse(diag, buf->matched_interval());
  // Attach symbol def if it exists.
  if (symbol) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::instruction() {
  auto buf = active_buffer();
  using ISA = isa::Pep10;
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  lex::Checkpoint cp(*buf);

  if (auto instruction = buf->match<lex::Identifier>(); !instruction) return cp.rollback(), nullptr;
  else if (auto instr = ISA::parseMnemonic(instruction->to_string()); instr == ISA::Mnemonic::INVALID) {
    throw PepParserError(PepParserError::UnaryError::Mnemonic_Invalid, instruction->to_string(),
                         buf->matched_interval());
  } else if (ISA::isMnemonicUnary(instr)) { // Monadic instruction
    ret = std::make_shared<MonadicInstruction>(Pep10Mnemonic(instr));
  } else { // Dyadic instruction
    ISA::AddressingMode am = ISA::AddressingMode::INVALID;
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, buf->matched_interval());
    else if (arg->minimum_size() > 2)
      throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, buf->matched_interval());
    // TODO: reject string arguments
    else if (buf->match_literal(",")) {
      auto addr_mode = buf->match<lex::Identifier>();
      if (!addr_mode)
        throw PepParserError(PepParserError::NullaryError::AddressingMode_Missing, buf->matched_interval());
      auto addr_mode_str = bits::to_upper(addr_mode->to_string());
      am = ISA::parseAddressingMode(addr_mode_str);
      if (am == ISA::AddressingMode::INVALID)
        throw PepParserError(PepParserError::NullaryError::AddressingMode_Invalid, buf->matched_interval());
      if (!ISA::isValidAddressingMode(instr, am))
        throw PepParserError(PepParserError::UnaryError::AddressingMode_InvalidForMnemonic, addr_mode_str,
                             buf->matched_interval());
    } else if (!ISA::requiresAddressingMode(instr)) am = ISA::defaultAddressingMode(instr);
    else throw PepParserError(PepParserError::NullaryError::AddressingMode_Required, buf->matched_interval());
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
    {"ELSE", (int)DC::ELSE},      {"ENDIF", (int)DC::ENDIF},   {"MACRO", (int)DC::INLINE_MACRO},
    {"ENDM", (int)DC::END_MACRO}};
} // namespace
std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::pseudo(OptionalSymbol symbol) {
  auto buf = active_buffer();
  auto lexer = active_lexer();
  auto dot = buf->match<lex::DotCommand>();
  if (!dot) return nullptr;
  auto dot_str = bits::to_upper(dot->to_string());
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend())
    throw PepParserError(PepParserError::UnaryError::Dot_Invalid, dot_str, buf->matched_interval());

  switch (it->second) {
  case (int)DC::ALIGN: {
    auto arg = numeric_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, buf->matched_interval());
    u16 value;
    bits::span<u8> byte_buf{(u8 *)&value, 2};
    (void)arg->serialize(byte_buf, bits::hostOrder());
    if (!(value == 1 || value == 2 || value == 4 || value == 8))
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedPowerOfTwo, buf->matched_interval());
    return std::make_shared<DotAlign>(DotAlign::Which::ByteCount, Argument{arg});
  }
  case (int)DC::ASCII: {
    if (auto maybeStr = buf->match<lex::StringConstant>(); !maybeStr)
      throw PepParserError(PepParserError::NullaryError::Argument_ExpectedString, buf->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()};
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)DC::BLOCK: {
    auto arg = numeric_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, buf->matched_interval());
    return std::make_shared<DotBlock>(Argument{arg});
  }
  case (int)DC::BYTE: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 1)
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded1Byte, buf->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      if (ident->minimum_size() > 1)
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded1Byte, buf->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{ident});
    } else throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, buf->matched_interval());
  }
  case (int)DC::EQUATE: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, buf->matched_interval());
    else if (arg->minimum_size() > 2)
      throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, buf->matched_interval());
    else if (!symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Required, buf->matched_interval());
    return std::make_shared<DotEquate>(SymbolDeclaration{*symbol}, Argument{arg});
  }
  case (int)PDC::EXPORT: {
    auto arg = identifier_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    arg->symbol()->binding = pepp::core::symbol::Binding::Global;
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::EXPORT, Argument{arg});
  }
  case (int)PDC::IMPORT: {
    auto arg = identifier_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    arg->symbol()->binding = pepp::core::symbol::Binding::Weak;
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::IMPORT, Argument{arg});
  }
  case (int)PDC::INPUT: {
    auto arg = identifier_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::INPUT, Argument{arg});
  }
  case (int)DC::ORG: {
    auto arg = hex_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedHex, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    return std::make_shared<DotOrg>(DotOrg::Behavior::ORG, Argument{arg});
  }
  case (int)PDC::OUTPUT: {
    auto arg = identifier_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::OUTPUT, Argument{arg});
  }
  case (int)PDC::SCALL: {
    auto arg = identifier_argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    return std::make_shared<DotAnnotate>(DotAnnotate::Which::SCALL, Argument{arg});
  }
  case (int)DC::SECTION: {
    if (auto maybeSecName = buf->match<lex::StringConstant>(); !maybeSecName)
      throw PepParserError(PepParserError::NullaryError::Section_StringName, buf->matched_interval());
    else if (!buf->match_literal(","))
      throw PepParserError(PepParserError::NullaryError::Section_TwoArgs, buf->matched_interval());
    else if (auto maybeFlags = buf->match<lex::StringConstant>(); !maybeFlags)
      throw PepParserError(PepParserError::NullaryError::Section_StringFlags, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
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
        throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, buf->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{ident});
    } else throw PepParserError(PepParserError::NullaryError::Argument_ExpectedInteger, buf->matched_interval());
  }
  case (int)DC::IF: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    bool matched = arg->value_as<i16>() != 0;
    _conditionals.emplace_back(
        ConditionalStack{.matched_any = matched, .matched_this_stmt = matched, .matched_else = false});
    return std::make_shared<DotConditional>(DotConditional::Behavior::IF, Argument{arg});
  }
  case (int)DC::ELSEIF: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    else if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElseif, buf->matched_interval());

    auto &tos = _conditionals.back();
    if (tos.matched_else) {
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElseif, buf->matched_interval());
    } else if (tos.matched_any) _conditionals.back().matched_this_stmt = false;
    else {
      tos.matched_this_stmt = (arg->value_as<i16>() != 0);
      tos.matched_any = tos.matched_any || tos.matched_this_stmt;
    }
    return std::make_shared<DotConditional>(DotConditional::Behavior::ELSEIF, Argument{arg});
  }
  case (int)DC::ELSE: {
    if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    else if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedElse, buf->matched_interval());

    auto &tos = _conditionals.back();
    if (tos.matched_else)
      throw PepParserError(PepParserError::NullaryError::Conditional_MultipleElse, buf->matched_interval());
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
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    if (_conditionals.empty())
      throw PepParserError(PepParserError::NullaryError::Conditional_UnmatchedEndif, buf->matched_interval());
    _conditionals.pop_back();
    return std::make_shared<DotConditional>(DotConditional::Behavior::ENDIF);
  }
  case (int)DC::INLINE_MACRO: {
    auto name = identifier_argument();
    if (!name) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    // Mark the start of the macro's arguments
    lex::Marker marker(*buf);
    // Consume tokens until the EoL is reached, then attempt to split into arguments.
    buf->match_until<lex::Empty, lex::EoF>();
    auto tokens = buf->matched_tokens_after(marker);
    std::vector<std::string> args;
    std::span<std::shared_ptr<pepp::tc::lex::Token> const> head, rest = tokens;
    while (!rest.empty()) {
      std::tie(head, rest) = pepp::tc::split_exclusive(rest, split_args);
      const auto first_loc = head.front()->location().lower(), last_loc = head.back()->location().upper();
      auto arg = lexer->view(support::LocationInterval(first_loc, last_loc));
      args.emplace_back(arg);
    }
    _active_macro_defs++;
    return std::make_shared<InlineMacroDefinition>(name->string(), args);
  }
  case (int)DC::END_MACRO: {
    throw PepParserError(PepParserError::NullaryError::Macro_UnmatchedEndm, buf->matched_interval());
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::line(DiagnosticTable &diag, OptionalSymbol symbol) {
  auto buf = active_buffer();
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto dot = pseudo(symbol); dot) ret = dot;
  else if (auto macro = this->macro(diag, symbol); macro) ret = macro;
  else if (auto instr = instruction(); instr) ret = instr;

  else return nullptr;

  if (auto comment = buf->match<lex::InlineComment>(); comment) ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  return ret;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::statement(DiagnosticTable &diag) {
  auto buf = active_buffer();
  auto lexer = active_lexer();
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;

  {
    // Limit lifetime of checkpoint to avoid clearing pushed-back token after skip loop.
    lex::Checkpoint cp(*buf);
    if (auto empty = buf->match<tc::lex::Empty>(); empty) {
      auto line = std::make_shared<EmptyLine>();
      line->source_interval = empty->location();
      return line;
    }
    if (auto comment = buf->match<tc::lex::InlineComment>(); comment) {
      auto line = std::make_shared<CommentLine>(Comment(*comment->value));
      line->source_interval = comment->location();
      ret = line;
    }
    // Avoid line() if no input remains (e.g., due to unterminated macro or conditional).
    else if (buf->input_remains()) {
      auto symbol = buf->match<lex::SymbolDeclaration>();
      if (symbol && symbol->to_string().length() > 7)
        throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_TooLong, buf->matched_interval());

      auto symbol_decl = symbol ? OptionalSymbol(_symtab->define(symbol->to_string())) : std::nullopt;
      ret = line(diag, symbol_decl);
      if (!ret) {
        auto next = buf->peek();
        throw PepParserError(PepParserError::UnaryError::Token_Invalid, next->repr(), buf->matched_interval());
      } else {
        ret->source_interval = buf->matched_interval();
      }
    }

    if (!buf->match<tc::lex::Empty>() && buf->input_remains())
      throw PepParserError(PepParserError::NullaryError::Token_MissingNewline, buf->matched_interval());
  }
  // Start skip loop _after_ parsing the statement which entered the skip loop.
  // This way we can preserve an invariant that the first token consumed by statement is a part of the returned IR line.
  // This is particularly helpful for macros definitions where we need to associate the macro IR object with its inline
  // body.
  auto start_depth = _conditionals.size();
  const auto start_ival = lexer->current_location();
  // Consume tokens directly from lexer without buffering to avoid buffer-clearing bugs.
  while ((_active_macro_defs > 0 || in_false_conditional()) && lexer->input_remains()) {
    auto token = lexer->next_token();
    if (_active_macro_defs > 0) {
      // Need to capture all body tokens! Else chaos ensues.
      buf->push_token(token);
      // Need to count start / ends of macro definitions.
      if (token && token->type() == lex::DotCommand::TYPE) {
        auto dot_str = bits::to_upper(token->to_string());
        if (dot_str == "MACRO") _active_macro_defs++;
        else if (dot_str == "ENDM") {
          if (lexer->input_remains()) {
            auto la2 = lexer->next_token();
            if (!la2 || la2->type() != lex::Empty::TYPE)
              throw PepParserError(PepParserError::NullaryError::Token_MissingNewline, buf->matched_interval());
            buf->push_token(la2);
          }
          _active_macro_defs--;
        }
      }

      if (_active_macro_defs == 0) {
        auto as_macro = std::dynamic_pointer_cast<InlineMacroDefinition>(ret);
        if (!as_macro) throw std::logic_error("Expected an InlineMacroDefinition");
        // Flush collected tokens so future statements parse normally.
        lex::Checkpoint cp(*buf);
        auto tokens = buf->buffered_tokens();

        auto macro_def = std::make_shared<MacroDefinition>();
        macro_def->name = as_macro->name;
        for (const auto &arg : as_macro->arguments)
          macro_def->arguments.emplace_back(MacroDefinition::Argument{.name = arg, .default_value = std::nullopt});

        // Strip final .ENDM NEWLINE token from macro body while handling edgecase of an empty body.
        if (tokens.size() > 2) {
          // Drop trailing .endm token from macro body.
          tokens = tokens.subspan(0, tokens.size() - 2);

          const auto first_loc = tokens.front()->location().lower(), last_loc = tokens.back()->location().upper();
          auto str = lexer->view(support::LocationInterval(first_loc, last_loc));
          macro_def->body = str;
        } else macro_def->body = "";

        auto success = _macros->insert(macro_def);
        if (!success)
          throw PepParserError(PepParserError::UnaryError::Macro_Redefinition, as_macro->name,
                               {start_ival, tokens.back()->location().upper()});
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
        buf->push_token(token);
        break;
      } else if (dot_str == "ENDIF") {
        if (start_depth < _conditionals.size()) _conditionals.pop_back();
        // Do not consume ENDIF token closing the conditional that entered skip mode. Re-buffer that token so we can
        // take a normal parsing path for it and emit the proper IR for the closing directive.
        else {
          buf->push_token(token);
          break;
        }
      }
    }
  }

  if (!buf->input_remains() && _active_macro_defs > 0) {
    const auto end_ival = lexer->current_location();
    support::LocationInterval ival{start_ival, end_ival};
    throw PepParserError(PepParserError::NullaryError::Macro_Unterminated, ival);
  } else if (!buf->input_remains() && _conditionals.size() > 0) {
    const auto end_ival = lexer->current_location();
    support::LocationInterval ival{start_ival, end_ival};
    throw PepParserError(PepParserError::NullaryError::Conditional_Unterminated, ival);
  }

  return ret;
}

void pepp::tc::parser::PepParser::synchronize() {
  auto buf = active_buffer();
  // Scan until we reach a newline.
  static const auto mask = ~(lex::Empty::TYPE | lex::EoF::TYPE);
  while (buf->input_remains() && buf->match(mask));
}

bool pepp::tc::parser::PepParser::in_false_conditional() const {
  return std::accumulate(_conditionals.begin(), _conditionals.end(), false,
                         [](bool acc, const ConditionalStack &cs) { return acc || (!cs.matched_this_stmt); });
}
