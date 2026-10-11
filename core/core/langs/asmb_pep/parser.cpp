#include "core/langs/asmb_pep/parser.hpp"
#include <optional>
#include <stdexcept>
#include <utility>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/macro/macro_replacement.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/lexer.hpp"
#include "core/langs/asmb_pep/parser_error.hpp"
#include "core/langs/asmb_pep/text_format.hpp"
#include "core/macros.hpp"
#include "core/langs/asmb/expression_operand.hpp"
#include "core/math/bitmanip/mask.hpp"
#include "core/math/bitmanip/strings.hpp"

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
  constexpr expr::Options options{.default_type = {16, expr::Signedness::Unsigned},
                                  .dot = expr::Options::Dot::Identifier};
  constexpr auto invalid = [](const expr::Error &error) {
    return PepParserError(PepParserError::UnaryError::Expression_Invalid, error.message(), error.location);
  };
  // Re-use existing location counter for this line if possible.
  const auto location_counter = [this] {
    if (!_location_counter) _location_counter = _symtab->location_counter();
    return std::string{_location_counter->name};
  };

  auto buf = active_buffer();
  // Must try expression first, otherwise we might lex part of the expression searching for a string constant.
  if (const auto expr = parse_expression(*buf, *active_lexer(), _pool, options, location_counter); !expr)
    throw invalid(expr.error());
  else if (*expr) {
    if (const auto value = lower(**expr, _symtab, options.default_type, 2); !value) throw invalid(value.error());
    else return *value;
  } else if (auto maybeStr = buf->match<lex::StringConstant>())
    return std::make_shared<pepp::ast::String>(std::string{maybeStr->view()});
  else return nullptr;
}

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::numeric_argument() {
  return std::dynamic_pointer_cast<pepp::ast::Numeric>(argument());
}

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::PepParser::hex_argument() {
  return std::dynamic_pointer_cast<pepp::ast::Hexadecimal>(argument());
}

std::shared_ptr<pepp::ast::Symbolic> pepp::tc::parser::PepParser::identifier_argument() {
  return std::dynamic_pointer_cast<pepp::ast::Symbolic>(argument());
}

namespace {
// Conditionals choose which lines are parsed, so like equates they are evaluated as they are parsed.
bool condition_holds(pepp::ast::IRValue &arg, pepp::tc::support::LocationInterval location) {
  using pepp::tc::PepParserError;

  if (const auto value = pepp::tc::parser::equate_value(arg, location); !value)
    throw PepParserError(PepParserError::UnaryError::Expression_Invalid, value.error().message(), value.error().location);
  else if (!*value) throw PepParserError(PepParserError::NullaryError::Conditional_NotConstant, location);
  else return (**value & bits::mask(2)) != 0;
}

pepp::tc::PepParserError conditional_error(pepp::tc::parser::Conditionals::Error error,
                                           pepp::tc::support::LocationInterval location) {
  using E = pepp::tc::parser::Conditionals::Error;
  using NE = pepp::tc::PepParserError::NullaryError;
  switch (error) {
  case E::UnmatchedElseif: return pepp::tc::PepParserError(NE::Conditional_UnmatchedElseif, location);
  case E::UnmatchedElse: return pepp::tc::PepParserError(NE::Conditional_UnmatchedElse, location);
  case E::MultipleElse: return pepp::tc::PepParserError(NE::Conditional_MultipleElse, location);
  case E::UnmatchedEndif: return pepp::tc::PepParserError(NE::Conditional_UnmatchedEndif, location);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::PepParserError macro_error(const pepp::tc::parser::MacroCapture::Error &error) {
  using K = pepp::tc::parser::MacroCapture::Error::Kind;
  using PE = pepp::tc::PepParserError;
  switch (error.kind) {
  case K::MissingNewline: return PE(PE::NullaryError::Token_MissingNewline, error.location);
  case K::Unterminated: return PE(PE::NullaryError::Macro_Unterminated, error.location);
  case K::Redefinition: return PE(PE::UnaryError::Macro_Redefinition, error.macro, error.location);
  }
  PEPP_UNREACHABLE();
}
} // namespace

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

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::macro(DiagnosticTable &diag, OptionalSymbol symbol) {
  auto buf = active_buffer();
  auto lexer = active_lexer();
  lex::Checkpoint cp(*buf);
  auto name = buf->match<lex::Identifier>();
  if (!name) return cp.rollback(), nullptr;
  auto definition = _macros->find(name->to_string());
  if (!definition) return cp.rollback(), nullptr;
  else cp.commit();

  // Consume all non-comments, non-empty tokens until the end of the current line.
  while (buf->match_not<tc::lex::Empty, tc::lex::EoF, tc::lex::InlineComment>());
  // The arguments are the text of the tokens after the macro name.
  const auto args = split_arguments(buf->matched_tokens_after(cp.marker()), *lexer);
  auto replacements = _counters.counters_for(definition->name);

  // TODO: Validate # of matched arguments vs number of args in definition, accounting for default values.
  for (int it = 0; it < definition->arguments.size(); it++) {
    const auto &argument = definition->arguments[it];
    replacements["\\" + argument.name] = it < args.size() ? args[it] : argument.default_value.value_or("");
  }

  auto new_body = bits::rtrimmed(replace_macro_arguments(definition->body, replacements));
  auto new_lexer = std::make_shared<lex::PepLexer>(_pool, support::SeekableData{std::move(new_body)});
  _lexer_stack.emplace(new_lexer, std::make_shared<lex::Buffer>(&*new_lexer));

  auto ret = std::make_shared<MacroInstantiation>(definition, args);
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
    else if (!symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Required, buf->matched_interval());
    // Equates are assigned values as they are parsed. Their symbolic arguments must already be defined.
    const auto value = equate_value(*arg, buf->matched_interval());
    if (!value)
      throw PepParserError(PepParserError::UnaryError::Expression_Invalid, value.error().message(), value.error().location);
    else if (!*value)
      throw PepParserError(PepParserError::NullaryError::Argument_SymbolicEquate, buf->matched_interval());
    else if (arg->minimum_size() > 2)
      throw PepParserError(PepParserError::NullaryError::Argument_Exceeded2Bytes, buf->matched_interval());
    (*symbol)->value = std::make_shared<pepp::core::symbol::ConstantValue>(
        bits::MaskedBits{.byteCount = 2, .bitPattern = **value & bits::mask(2), .mask = bits::mask(2)});
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
    _conditionals.on_if(condition_holds(*arg, buf->matched_interval()));
    return std::make_shared<DotConditional>(DotConditional::Behavior::IF, Argument{arg});
  }
  case (int)DC::ELSEIF: {
    auto arg = argument();
    if (!arg) throw PepParserError(PepParserError::NullaryError::Argument_Missing, buf->matched_interval());
    else if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    const auto holds = [&] { return condition_holds(*arg, buf->matched_interval()); };
    if (const auto ok = _conditionals.on_elseif(holds); !ok)
      throw conditional_error(ok.error(), buf->matched_interval());
    return std::make_shared<DotConditional>(DotConditional::Behavior::ELSEIF, Argument{arg});
  }
  case (int)DC::ELSE: {
    if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    else if (const auto ok = _conditionals.on_else(); !ok)
      throw conditional_error(ok.error(), buf->matched_interval());
    return std::make_shared<DotConditional>(DotConditional::Behavior::ELSE);
  }
  case (int)DC::ENDIF: {
    if (symbol)
      throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_Forbidden, buf->matched_interval());
    else if (const auto ok = _conditionals.on_endif(); !ok)
      throw conditional_error(ok.error(), buf->matched_interval());
    return std::make_shared<DotConditional>(DotConditional::Behavior::ENDIF);
  }
  case (int)DC::INLINE_MACRO: {
    // A macro's name is not a symbol, and may use characters which expressions cannot (e.g., @TEST).
    auto name = buf->match<lex::Identifier>();
    if (!name) throw PepParserError(PepParserError::NullaryError::Argument_ExpectedIdentifier, buf->matched_interval());
    // Mark the start of the macro's arguments
    lex::Marker marker(*buf);
    // Consume tokens until the EoL is reached, then attempt to split into arguments.
    buf->match_until<lex::Empty, lex::EoF>();
    auto definition = std::make_shared<InlineMacroDefinition>(
        name->to_string(), split_arguments(buf->matched_tokens_after(marker), *lexer));
    // The body follows, and is read once this line is parsed.
    _macro_capture.begin(definition);
    return definition;
  }
  case (int)DC::END_MACRO: {
    throw PepParserError(PepParserError::NullaryError::Macro_UnmatchedEndm, buf->matched_interval());
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

namespace {
// RAII helper to prevent a macro expansion for sharing its host line's location counter.
struct Restore {
  std::shared_ptr<pepp::core::symbol::Entry> &slot, saved;
  ~Restore() { slot = std::move(saved); }
};
} // namespace

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::PepParser::line(DiagnosticTable &diag, OptionalSymbol symbol) {
  // This line gets its own location counter, and a macro expanded from it does not share the line's.
  Restore restore{_location_counter, std::exchange(_location_counter, nullptr)};
  auto buf = active_buffer();
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto dot = pseudo(symbol); dot) ret = dot;
  else if (auto macro = this->macro(diag, symbol); macro) ret = macro;
  else if (auto instr = instruction(); instr) ret = instr;

  else return nullptr;

  if (auto comment = buf->match<lex::InlineComment>(); comment) ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));

  if (_location_counter) { // If the line declared a symbol, alias the location counter to it.
    if (symbol) _location_counter->value = std::make_shared<core::symbol::AliasValue>(2, *symbol);
    ret->insert(std::make_unique<LocationCounterDeclaration>(_location_counter));
  }
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
      if (symbol && symbol->to_string().length() > 8)
        throw PepParserError(PepParserError::NullaryError::SymbolDeclaration_TooLong, buf->matched_interval());

      auto symbol_decl = symbol ? OptionalSymbol(_symtab->define(symbol->to_string())) : std::nullopt;
      // Lookahead for a comment or newline, which would indicate that this is a symbol-only line.
      // Symbol-only lines share a prefix with "normal" lines with symbols
      if (auto maybe_comment = buf->match<tc::lex::InlineComment>();
          symbol_decl && (buf->peek<tc::lex::Empty>() || maybe_comment)) {
        ret = std::make_shared<SymbolLine>(SymbolDeclaration{symbol_decl.value()});
        if (maybe_comment) ret->insert(std::make_unique<Comment>(*maybe_comment->value));
      } else ret = line(diag, symbol_decl);
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
  if (_macro_capture.capturing()) {
    if (const auto ok = _macro_capture.capture(*lexer, *_macros); !ok) throw macro_error(ok.error());
  }

  const auto start_depth = _conditionals.depth();
  const auto start_ival = lexer->current_location();
  // Consume tokens directly from lexer without buffering to avoid buffer-clearing bugs.
  while (_conditionals.skipping() && lexer->input_remains()) {
    auto token = lexer->next_token();
    // Do not consume a directive which may end the skip, so that it is parsed and emits its IR line.
    if (token && _conditionals.resumes_at(*token, start_depth)) {
      buf->push_token(token);
      break;
    }
  }

  if (!buf->input_remains() && _conditionals.depth() > 0) {
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

pepp::tc::IRProgram pepp::tc::parser::flatten_macros(const IRProgram &program, bool macro_comments) {
  const auto comments = macro_comments ? std::optional(MacroComments{format_as_columns, ';'}) : std::nullopt;
  return flatten_macros(program, comments);
}
