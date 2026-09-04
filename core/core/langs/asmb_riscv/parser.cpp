#include "core/langs/asmb_riscv/parser.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/parser_error.hpp"
#include "core/math/bitmanip/strings.hpp"

pepp::tc::parser::RISCVParser::RISCVParser(support::SeekableData &&data)
    : _pool(std::make_shared<std::unordered_set<std::string>>()),
      _lexer(std::make_shared<langs::RISCVLexer>(_pool, std::move(data))),
      _buffer(std::make_shared<lex::Buffer>(&*_lexer)), _symtab(std::make_shared<pepp::core::symbol::LeafTable>(2)) {}

std::shared_ptr<pepp::core::symbol::LeafTable> pepp::tc::parser::RISCVParser::symbol_table() const { return _symtab; }

pepp::tc::IRProgram pepp::tc::parser::RISCVParser::parse(DiagnosticTable &diag) {
  IRProgram lines;
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
      return std::make_shared<pepp::ast::SignedDecimal>(maybeInteger->value, 4);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pepp::ast::Hexadecimal>(maybeInteger->value, 4);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pepp::ast::UnsignedDecimal>(maybeInteger->value, 4);
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

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::RISCVParser::numeric_argument() {
  auto arg = argument();
  return std::dynamic_pointer_cast<pepp::ast::Numeric>(arg);
}

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::RISCVParser::hex_argument() {
  auto arg = argument();
  return std::dynamic_pointer_cast<pepp::ast::Hexadecimal>(arg);
}

std::shared_ptr<pepp::ast::Symbolic> pepp::tc::parser::RISCVParser::identifier_argument() {
  auto arg = argument();
  return std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg);
}

namespace {
using D = riscv::Operand::Destination;
using E = pepp::tc::RISCVParserError::NullaryError;

// What error should be returned if a field is expected but not present?
E expected_error(D destination) {
  switch (destination) {
  case D::RD: return E::Argument_ExpectedRD;
  case D::RS: [[fallthrough]];
  case D::RS1: return E::Argument_ExpectedRS1;
  case D::RS2: return E::Argument_ExpectedRS2;
  case D::IMM: [[fallthrough]];
  case D::SHAMT: return E::Argument_ExpectedImm;
  case D::PRED: [[fallthrough]];
  case D::SUCC: [[fallthrough]];
  case D::Invalid: break;
  }
  return E::Argument_ExpectedIdentNumeric;
}

// N
void store_value(pepp::tc::ParsedOperands &values, D destination, u8 v) {
  switch (destination) {
  case D::RD: values.rd = v; break;
  // RS is the lone source of a two-operand pseudo, occupying the RS1 position.
  case D::RS: [[fallthrough]];
  case D::RS1: values.rs1 = v; break;
  case D::RS2: values.rs2 = v; break;
  case D::PRED: values.pred = v; break;
  case D::SUCC: values.succ = v; break;
  case D::IMM: [[fallthrough]];
  case D::SHAMT: [[fallthrough]];
  case D::Invalid: throw std::logic_error("Unreachable path on store_value");
  }
}

} // namespace

// Attempt to parse one alternative of an instruction, returning a node if successful. If it fails, the buffer will be
// rolled back. An error should only be treated as fatal by instruction() if there are no further alternatives to try.
std::shared_ptr<pepp::tc::IntegerInstruction>
pepp::tc::parser::RISCVParser::instruction_alternative(const riscv::Mnemonic &entry) {
  using OT = riscv::Operand::Type;
  using RVPE = RISCVParserError;
  const auto &desc = entry.mn;

  lex::Checkpoint cp(*_buffer);
  // May contain a pointer to a Symbol via imm. On failed parse, we should undefine that symbol.
  ParsedOperands values;
  try {
    // Parsing is entirely driven by the descriptor, including the operand order and separator after a field.
    const auto operands = desc.operands();
    for (std::size_t i = 0; i < operands.size(); ++i) {
      const auto &operand = operands[i];
      switch (operand.type) {
      case OT::Register: {
        const auto reg = register_integer();
        if (!reg) throw RVPE(expected_error(operand.destination), _buffer->matched_interval());
        store_value(values, operand.destination, *reg);
        break;
      }

      // Handle registers surrounded by parens, which are used for loads+stores
      case OT::ParenthesizedRegister: {
        if (!_buffer->match_literal("(")) throw RVPE(E::Token_MissingLParen, _buffer->matched_interval());
        else if (const auto reg = register_integer(); !reg)
          throw RVPE(expected_error(operand.destination), _buffer->matched_interval());
        else if (!_buffer->match_literal(")")) throw RVPE(E::Token_MissingRParen, _buffer->matched_interval());
        else store_value(values, operand.destination, *reg);
        break;
      }
      case OT::Immediate: {
        if (auto arg = argument(); !arg) throw RVPE(::expected_error(operand.destination), _buffer->matched_interval());
        else values.imm = arg;
        break;
      }
      // Constants not actually parsed; set automatically by MnemonicDecoder.
      case OT::XLEN8: [[fallthrough]];
      case OT::XLEN16: break;
      // Parse pred,succ into specialized field of ParsedValue to avoid read-modify-write on IRValue.
      case OT::Fence: {
        std::optional<u8> ordering = std::nullopt;
        // Ordering could be identifier iorw or integer 0; need to parse both.
        if (const auto token = _buffer->match<lex::Identifier>()) {
          auto text = token->to_string();
          bits::to_lower_inplace(text);
          ordering = riscv::parse_fence_ordering(text);
        } else if (const auto number = _buffer->match<lex::Integer>(); number && number->value == 0) ordering = 0;
        if (!ordering) throw RVPE(E::Argument_ExpectedFenceOrdering, _buffer->matched_interval());
        else store_value(values, operand.destination, *ordering);
        break;
      }
      case OT::Invalid: break;
      }

      // Operands are comma-separated unless the descriptor says otherwise.
      if (i + 1 < operands.size() && desc.comma_after(i) && !_buffer->match_literal(","))
        throw RVPE(E::Token_MissingComma, _buffer->matched_interval());
    }

    // Ensure that this instruction will match one of our line end types, to prevent a matched prefix from failing on
    // later steps.
    static constexpr int line_end = lex::InlineComment::TYPE | lex::Empty::TYPE | lex::EoF::TYPE;
    if (_buffer->input_remains() && !_buffer->peek(line_end))
      throw RVPE(E::Token_MissingNewline, _buffer->matched_interval());

    // Null for Pseudo and INVALID, which have no single node to build; nothing was really matched.
    auto built = make_instruction(entry.name, desc, values);
    if (!built) cp.rollback();
    return built;
  } catch (...) {
    cp.rollback();
    // If we created a symbolic reference AND this is the only usage of that symbol, drop it from the symbol table.
    // This precludes ordering issues between the JAL variants `jal ra, off` and `jal off`. If the first is parsed as
    // the second, `ra` would end up as a declared symbol with no definition.
    if (auto symbolic = std::dynamic_pointer_cast<pepp::ast::Symbolic>(values.imm); symbolic) {
      const auto name = symbolic->symbol()->name;
      values.imm.reset();
      symbolic.reset();
      _symtab->drop(name);
    }
    throw;
  }
}

std::shared_ptr<pepp::tc::IntegerInstruction> pepp::tc::parser::RISCVParser::instruction() {
  lex::Checkpoint cp(*_buffer);
  const auto maybe_instr = _buffer->match<lex::Identifier>();
  if (!maybe_instr) return cp.rollback(), nullptr;
  auto instr_str = maybe_instr->to_string();
  bits::to_lower_inplace(instr_str);
  const auto [first, last] = riscv::string_to_mnemonic.equal_range(instr_str);
  if (first == last) return cp.rollback(), nullptr;

  // A mnemonic may have several possible alternatives, such as `jal rd, offset` and `jal offset`.
  for (auto candidate = first; candidate != last; ++candidate) {
    const bool is_last = std::next(candidate) == last;
    try {
      if (auto built = instruction_alternative(*candidate); built) return built;
    } catch (const RISCVParserError &) {
      // Inner parser's error is only fatal if we have exhausted all possible variants.
      if (is_last) throw;
    }
  }
  // Failed to parse any alternative.
  return cp.rollback(), nullptr;
}

namespace {
using DC = pepp::tc::DotCommands;
using LDC = pepp::tc::RISCVDotCommands;
static const auto dot_map = std::map<std::string, int>{
    {"ASCII", (int)DC::ASCII},
    {"ASCIZ", (int)LDC::ASCIZ},
    {"BALIGN", (int)LDC::ALIGN_BYTE},
    {"BLOCK", (int)DC::BLOCK},
    {"BYTE", (int)DC::BYTE},
    {"EQUATE", (int)DC::EQUATE},
    {"GLOBAL", (int)LDC::SYMBOL_GLOBAL},
    {"HALF", (int)DC::HALF},
    {"HIDDEN", (int)LDC::SYMBOL_HIDDEN},
    {"LOCAL", (int)LDC::SYMBOL_LOCAL},
    {"ORG", (int)DC::ORG},
    {"P2ALIGN", (int)LDC::ALIGN_P2},
    {"SECTION", (int)DC::SECTION},
    {"WEAK", (int)LDC::SYMBOL_WEAK},
    {"WORD", (int)DC::WORD},
    // Aliases for .SECTION
    {"TEXT", (int)LDC::SECTION_TEXT},
    {"BSS", (int)LDC::SECTION_BSS},
    {"RODATA", (int)LDC::SECTION_RODATA},
    {"DATA", (int)LDC::SECTION_DATA},
    // Aliases for previous directives
    // On RISC-V targets, aligns are treated as powers-of-2 by default.
    {"ALIGN", {(int)LDC::ALIGN_P2}},
    {"EQU", (int)DC::EQUATE},
    {"GLOBL", (int)LDC::SYMBOL_GLOBAL},
    {"SET", (int)DC::EQUATE},
    {"SKIP", (int)DC::BLOCK},
    {"STRING", (int)LDC::ASCIZ},
    {"ZERO", (int)DC::BLOCK},
};
} // namespace

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::RISCVParser::pseudo(OptionalSymbol symbol) {
  auto dot = _buffer->match<lex::DotCommand>();
  if (!dot) return nullptr;
  auto dot_str = bits::to_upper(dot->to_string());
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend())
    throw RISCVParserError(RISCVParserError::UnaryError::Dot_Invalid, dot_str, _buffer->matched_interval());

  switch (it->second) {
  case (int)LDC::ALIGN_P2: [[fallthrough]];
  case (int)LDC::ALIGN_BYTE: {
    auto arg = numeric_argument();
    if (!arg)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    u16 value;
    bits::span<u8> buf{(u8 *)&value, 2};
    (void)arg->serialize(buf, bits::hostOrder());
    DotAlign::Which w = (it->second == (int)LDC::ALIGN_P2) ? DotAlign::Which::Pow2 : DotAlign::Which::ByteCount;
    return std::make_shared<DotAlign>(w, Argument{arg});
  }
  case (int)LDC::SYMBOL_GLOBAL: [[fallthrough]];
  case (int)LDC::SYMBOL_HIDDEN: [[fallthrough]];
  case (int)LDC::SYMBOL_LOCAL: [[fallthrough]];
  case (int)LDC::SYMBOL_WEAK: {

    auto arg = identifier_argument();
    if (!arg)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
    DotSymbol::Which w;
    if (it->second == (int)LDC::SYMBOL_GLOBAL) {
      w = DotSymbol::Which::Global;
      arg->symbol()->binding = pepp::core::symbol::Binding::Global;
    } else if (it->second == (int)LDC::SYMBOL_HIDDEN) {
      w = DotSymbol::Which::Hidden;
      arg->symbol()->visibility = pepp::core::symbol::Visibility::Hidden;
    } else if (it->second == (int)LDC::SYMBOL_LOCAL) {
      w = DotSymbol::Which::Local;
      arg->symbol()->binding = pepp::core::symbol::Binding::Local;
    } else {
      w = DotSymbol::Which::Weak;
      arg->symbol()->binding = pepp::core::symbol::Binding::Weak;
    }

    return std::make_shared<DotSymbol>(w, Argument{arg});
  }

  case (int)DC::ASCII: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()};
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)LDC::ASCIZ: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()} + '\0';
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)DC::BLOCK: {
    auto arg = numeric_argument();
    if (!arg)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    return std::make_shared<DotBlock>(Argument{arg});
  }
  case (int)DC::BYTE: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 1)
        throw RISCVParserError(RISCVParserError::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      if (ident->minimum_size() > 1)
        throw RISCVParserError(RISCVParserError::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{ident});
    } else
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::HALF: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 2)
        throw RISCVParserError(RISCVParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{ident});
    } else
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::WORD: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 4)
        throw RISCVParserError(RISCVParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte4, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte4, Argument{ident});
    } else
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::EQUATE: {
    auto arg = argument();
    if (!arg)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    else if (arg->minimum_size() > 2)
      throw RISCVParserError(RISCVParserError::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    else if (!symbol)
      throw RISCVParserError(RISCVParserError::NullaryError::SymbolDeclaration_Required, _buffer->matched_interval());
    return std::make_shared<DotEquate>(SymbolDeclaration{*symbol}, Argument{arg});
  }
  case (int)DC::ORG: {
    auto arg = hex_argument();
    if (!arg) throw RISCVParserError(RISCVParserError::NullaryError::Argument_ExpectedHex, _buffer->matched_interval());
    else if (symbol)
      throw RISCVParserError(RISCVParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotOrg>(DotOrg::Behavior::ORG, Argument{arg});
  }
  case (int)DC::SECTION: {
    if (auto maybeSecName = _buffer->match<lex::StringConstant>(); !maybeSecName)
      throw RISCVParserError(RISCVParserError::NullaryError::Section_StringName, _buffer->matched_interval());
    else if (!_buffer->match_literal(","))
      throw RISCVParserError(RISCVParserError::NullaryError::Section_TwoArgs, _buffer->matched_interval());
    else if (auto maybeFlags = _buffer->match<lex::StringConstant>(); !maybeFlags)
      throw RISCVParserError(RISCVParserError::NullaryError::Section_StringFlags, _buffer->matched_interval());
    else if (symbol)
      throw RISCVParserError(RISCVParserError::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    else {
      auto flags = bits::to_lower(maybeFlags->view());
      using bits::contains;
      bool r = contains(flags, "r"), w = contains(flags, "w"), x = contains(flags, "x"), z = contains(flags, "z");
      return std::make_shared<DotSection>(Identifier(*maybeSecName->value), SectionFlags(r, w, x, z));
    }
  }
  case (int)LDC::SECTION_TEXT: {
    static const std::string name = ".text";
    _pool->insert(name);
    SectionFlags flags(true, false, true, false);
    return std::make_shared<DotSection>(Identifier(name), flags);
  }
  case (int)LDC::SECTION_BSS: {
    static const std::string name = ".bss";
    _pool->insert(name);
    SectionFlags flags(true, true, false, true);
    return std::make_shared<DotSection>(Identifier(name), flags);
  }
  case (int)LDC::SECTION_RODATA: {
    static const std::string name = ".rodata";
    _pool->insert(name);
    SectionFlags flags(true, false, false, false);
    return std::make_shared<DotSection>(Identifier(name), flags);
  }
  case (int)LDC::SECTION_DATA: {
    static const std::string name = ".data";
    _pool->insert(name);
    SectionFlags flags(true, true, false, false);
    return std::make_shared<DotSection>(Identifier(name), flags);
  }
  default: throw std::logic_error("Unreachable");
  }
  return nullptr;
}

std::shared_ptr<pepp::tc::LinearIR> pepp::tc::parser::RISCVParser::line(OptionalSymbol symbol) {
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
