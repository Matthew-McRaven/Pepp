#include "core/langs/asmb_riscv/parser.hpp"
#include <array>
#include <expected>
#include <utility>
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/expression_operand.hpp"
#include "core/langs/asmb_riscv/parser_error.hpp"
#include "core/math/bitmanip/mask.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
namespace expr = pepp::tc::expr;

// The upper 20 bits shifted into the lower 20 bits. Must add +0x800.
// e.g., 0x12345FFF. Without addition, lo=0xFFF, hi=x012345000
// lo is sign extend to 0xFFFF'FFFF, so a lui/addi pair would compute 12344FFF
// So we need to round hi up when lo is negative (bit 11 / 0x800)is set.
std::expected<expr::Value, expr::NullaryError> hi20(expr::Value x, expr::Type) {
  return expr::Value{(((x.bits & 0xFFFF'FFFF) + 0x800) >> 12) & 0xF'FFFF, {32, expr::Signedness::Unsigned}};
}
expr::Type hi_type(expr::Type, expr::Type) { return {32, expr::Signedness::Unsigned}; }

// Sign-extended lower 12 bits.
std::expected<expr::Value, expr::NullaryError> lo12(expr::Value x, expr::Type) {
  // branch-free sign extension of 12-bit quantity to 32-bits.
  const auto sign_extended = (((x.bits & 0xFFF) ^ 0x800) - 0x800);
  return expr::Value{sign_extended & 0xFFFF'FFFF, {32, expr::Signedness::Signed}};
}
expr::Type lo_type(expr::Type, expr::Type) { return {32, expr::Signedness::Signed}; }

// Currently implemented relocation modifiers
constexpr std::array<expr::Function, 4> functions{{
    {"%hi", hi20, hi_type},
    {"%lo", lo12, lo_type},
    {"%pcrel_hi", nullptr, hi_type},
    {"%pcrel_lo", nullptr, lo_type},
}};

// `.` is the location counter, and %name a relocation modifier.
constexpr expr::Options options{.default_type = {32, expr::Signedness::Signed},
                                .dot = expr::Options::Dot::Identifier,
                                .percent_identifiers = true,
                                .functions = functions};

// Re-use existing location counter for this line if possible.
pepp::tc::expr::NameLocationCounter resolve_location_counter(pepp::core::symbol::LeafTable &symtab,
                                                             std::shared_ptr<pepp::core::symbol::Entry> &counter) {
  return [&symtab, &counter] {
    if (!counter) counter = symtab.location_counter();
    return std::string{counter->name};
  };
}
} // namespace

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

std::shared_ptr<pepp::ast::IRValue> pepp::tc::parser::RISCVParser::argument() {
  constexpr auto invalid = [](const expr::Error &error) {
    return RISCVParserError(RISCVParserError::UnaryError::Expression_Invalid, error.message(), error.location);
  };
  const auto location_counter = resolve_location_counter(*_symtab, _location_counter);

  if (const auto expr = parse_expression(*_buffer, *_lexer, _pool, options, location_counter); !expr)
    throw invalid(expr.error());
  else if (*expr) {
    if (const auto value = lower(**expr, _symtab, options.default_type, 4); !value) throw invalid(value.error());
    else return *value;
  } else if (auto maybeStr = _buffer->match<lex::StringConstant>())
    return std::make_shared<pepp::ast::String>(std::string{maybeStr->view()});
  else return nullptr;
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

namespace {
// A register written as an identifier, or in parentheses for loads and stores. Do not reference identifier in symtab.
std::optional<u8> register_of(const pepp::tc::expr::Tree &tree, bool parenthesized) {
  namespace expr = pepp::tc::expr;
  const auto &kinds = tree.kinds();
  const bool shaped = parenthesized ? kinds.size() == 2 && kinds[1] == expr::Kind::Parens : kinds.size() == 1;
  if (const auto *identifier = std::get_if<expr::Identifier>(&tree[0]); shaped && identifier)
    return riscv::parse_register(identifier->name);
  return std::nullopt;
}

// A fence's predecessor or successor set, written as e.g. iorw, or as 0.
std::optional<u8> fence_ordering_of(const pepp::tc::expr::Tree &tree) {
  namespace expr = pepp::tc::expr;
  if (tree.kinds().size() != 1) return std::nullopt;
  else if (const auto *identifier = std::get_if<expr::Identifier>(&tree[0]))
    return riscv::parse_fence_ordering(bits::to_lower(identifier->name));
  else if (const auto *integer = std::get_if<expr::Integer>(&tree[0]); integer && integer->value == 0) return 0;
  return std::nullopt;
}

// False for operands which fill a field automatically (e.g., XLEN8 sets the immediate to 8).
bool appears_in_source(riscv::Operand::Type type) {
  using OT = riscv::Operand::Type;
  return type != OT::XLEN8 && type != OT::XLEN16 && type != OT::Invalid;
}

// Why the written operands do not fit a pattern, or nullopt if they do. Nothing is lowered, so trying a pattern has no
// effect on the symbol table.
std::optional<E> mismatch(const riscv::MnemonicDescriptor &desc, std::span<const pepp::tc::parser::RISCVOperand> ops) {
  using OT = riscv::Operand::Type;
  const auto operands = desc.operands();
  std::size_t next = 0, previous = 0;
  for (std::size_t i = 0; i < operands.size(); ++i) {
    const auto &operand = operands[i];
    if (!appears_in_source(operand.type)) continue;
    else if (next == ops.size()) return expected_error(operand.destination);
    // Operands are comma-separated unless the descriptor says otherwise.
    if (const bool comma = ops[next].comma_before; next > 0 && comma != desc.comma_after(previous))
      return comma ? expected_error(operand.destination) : E::Token_MissingComma;
    const auto &tree = ops[next].expression->tree;
    if (operand.type == OT::Register && !register_of(tree, false)) return expected_error(operand.destination);
    else if (operand.type == OT::ParenthesizedRegister && !register_of(tree, true))
      return expected_error(operand.destination);
    else if (operand.type == OT::Fence && !fence_ordering_of(tree)) return E::Argument_ExpectedFenceOrdering;
    previous = i, next++;
  }
  if (next != ops.size()) return E::Token_MissingNewline;
  return std::nullopt;
}
} // namespace

std::vector<pepp::tc::parser::RISCVOperand> pepp::tc::parser::RISCVParser::mnemonic_operands() {
  constexpr auto invalid = [](const expr::Error &error) {
    return RISCVParserError(RISCVParserError::UnaryError::Expression_Invalid, error.message(), error.location);
  };
  const auto location_counter = resolve_location_counter(*_symtab, _location_counter);
  std::vector<RISCVOperand> ret;
  bool comma = false;
  // Try an expression before a comma, so that the lexer never lexes an operand's text. An operand may also follow the
  // previous one without a comma, as (x3) does in 0(x3).
  while (true) {
    if (const auto expr = parse_expression(*_buffer, *_lexer, _pool, options, location_counter); !expr)
      throw invalid(expr.error());
    else if (*expr) ret.push_back({*expr, std::exchange(comma, false)});
    else if (!comma && _buffer->match_literal(",")) comma = true;
    else break;
  }
  if (comma) throw RISCVParserError(E::Argument_ExpectedIdentNumeric, _buffer->matched_interval());
  return ret;
}

std::shared_ptr<pepp::tc::IntegerInstruction>
pepp::tc::parser::RISCVParser::match_alternative(const riscv::Mnemonic &entry, std::span<const RISCVOperand> ops) {
  using RVPE = RISCVParserError;
  using OT = riscv::Operand::Type;
  ParsedOperands values;
  std::size_t next = 0;
  for (const auto &operand : entry.mn.operands()) {
    if (!appears_in_source(operand.type)) continue;
    const auto &expression = *ops[next++].expression;
    switch (operand.type) {
    case OT::Register: [[fallthrough]];
    case OT::ParenthesizedRegister: {
      const bool parenthesized = operand.type == OT::ParenthesizedRegister;
      store_value(values, operand.destination, *register_of(expression.tree, parenthesized));
      break;
    }
    case OT::Fence: store_value(values, operand.destination, *fence_ordering_of(expression.tree)); break;
    case OT::Immediate:
      if (auto value = lower(expression, _symtab, options.default_type, 4); !value)
        throw RVPE(RVPE::UnaryError::Expression_Invalid, value.error().message(), value.error().location);
      else values.imm = *value;
      break;
    default: break;
    }
  }
  // Null for Pseudo and INVALID.
  return make_instruction(entry.name, entry.mn, values);
}

std::shared_ptr<pepp::tc::IntegerInstruction> pepp::tc::parser::RISCVParser::instruction() {
  lex::Checkpoint cp(*_buffer);
  const auto maybe_instr = _buffer->match<lex::Identifier>();
  if (!maybe_instr) return cp.rollback(), nullptr;
  auto instr_str = maybe_instr->to_string();
  bits::to_lower_inplace(instr_str);
  const auto [first, last] = riscv::string_to_mnemonic.equal_range(instr_str);
  if (first == last) return cp.rollback(), nullptr;

  // Parse the operands once, then find the pattern that fits
  const auto ops = mnemonic_operands();
  std::optional<E> problem;
  bool fit = false;
  for (auto candidate = first; candidate != last; ++candidate) {
    if (const auto why = mismatch(candidate->mn, ops)) problem = why;
    else if (fit = true; auto built = match_alternative(*candidate, ops)) return built;
  }
  // Report why the most recent pattern did not fit. If there was a fit but it returned nullptr, roll back.
  if (!fit && problem) throw RISCVParserError(*problem, _buffer->matched_interval());
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
  using RVPE = RISCVParserError;
  auto dot = _buffer->match<lex::DotCommand>();
  if (!dot) return nullptr;
  auto dot_str = bits::to_upper(dot->to_string());
  auto it = dot_map.find(dot_str);
  if (it == dot_map.cend()) throw RVPE(RVPE::UnaryError::Dot_Invalid, dot_str, _buffer->matched_interval());

  switch (it->second) {
  case (int)LDC::ALIGN_P2: [[fallthrough]];
  case (int)LDC::ALIGN_BYTE: {
    auto arg = numeric_argument();
    if (!arg) throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
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
    if (!arg) throw RVPE(RVPE::NullaryError::Argument_ExpectedIdentifier, _buffer->matched_interval());
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
      throw RVPE(RVPE::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()};
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)LDC::ASCIZ: {
    if (auto maybeStr = _buffer->match<lex::StringConstant>(); !maybeStr)
      throw RVPE(RVPE::NullaryError::Argument_ExpectedString, _buffer->matched_interval());
    else {
      const auto asStr = std::string{maybeStr->view()} + '\0';
      Argument arg{std::make_shared<pepp::ast::String>(asStr)};
      return std::make_shared<DotLiteral>(DotLiteral::Which::ASCII, arg);
    }
  }
  case (int)DC::BLOCK: {
    auto arg = numeric_argument();
    if (!arg) throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    return std::make_shared<DotBlock>(Argument{arg});
  }
  case (int)DC::BYTE: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 1)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      if (ident->minimum_size() > 1)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{ident});
    } else if (auto expression = std::dynamic_pointer_cast<pepp::ast::Expression>(arg); expression) {
      if (expression->minimum_size() > 1)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded1Byte, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte1, Argument{expression});
    } else
      throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::HALF: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 2)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{ident});
    } else if (auto expression = std::dynamic_pointer_cast<pepp::ast::Expression>(arg); expression) {
      if (expression->minimum_size() > 2)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte2, Argument{expression});
    } else
      throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::WORD: {
    auto arg = argument();
    if (auto numeric = std::dynamic_pointer_cast<pepp::ast::Numeric>(arg); numeric) {
      if (numeric->minimum_size() > 4)
        throw RVPE(RVPE::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte4, Argument{numeric});
    } else if (auto ident = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg); ident) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte4, Argument{ident});
    } else if (auto expression = std::dynamic_pointer_cast<pepp::ast::Expression>(arg); expression) {
      return std::make_shared<DotLiteral>(DotLiteral::Which::Byte4, Argument{expression});
    } else
      throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
  }
  case (int)DC::EQUATE: {
    auto arg = argument();
    if (!arg) throw RVPE(RVPE::NullaryError::Argument_ExpectedInteger, _buffer->matched_interval());
    else if (arg->minimum_size() > 2)
      throw RVPE(RVPE::NullaryError::Argument_Exceeded2Bytes, _buffer->matched_interval());
    else if (!symbol) throw RVPE(RVPE::NullaryError::SymbolDeclaration_Required, _buffer->matched_interval());
    // Equates are assigned values as they are parsed. s:.EQUATE y creates an alias which may be a forward reference.
    // s: .EQUATE x+y requires that x and y be previously defined constants.
    if (auto symbolic = std::dynamic_pointer_cast<pepp::ast::Symbolic>(arg)) {
      (*symbol)->value = std::make_shared<pepp::core::symbol::AliasValue>(4, symbolic->symbol());
    } else if (const auto value = equate_value(*arg, _buffer->matched_interval()); !value) {
      throw RVPE(RVPE::UnaryError::Expression_Invalid, value.error().message(), value.error().location);
    } else if (!*value) {
      throw RVPE(RVPE::NullaryError::Argument_SymbolicEquate, _buffer->matched_interval());
    } else {
      (*symbol)->value = std::make_shared<pepp::core::symbol::ConstantValue>(
          bits::MaskedBits{.byteCount = 4, .bitPattern = **value & bits::mask(4), .mask = bits::mask(4)});
    }
    return std::make_shared<DotEquate>(SymbolDeclaration{*symbol}, Argument{arg});
  }
  case (int)DC::ORG: {
    auto arg = hex_argument();
    if (!arg) throw RVPE(RVPE::NullaryError::Argument_ExpectedHex, _buffer->matched_interval());
    else if (symbol) throw RVPE(RVPE::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
    return std::make_shared<DotOrg>(DotOrg::Behavior::ORG, Argument{arg});
  }
  case (int)DC::SECTION: {
    if (auto maybeSecName = _buffer->match<lex::StringConstant>(); !maybeSecName)
      throw RVPE(RVPE::NullaryError::Section_StringName, _buffer->matched_interval());
    else if (!_buffer->match_literal(",")) throw RVPE(RVPE::NullaryError::Section_TwoArgs, _buffer->matched_interval());
    else if (auto maybeFlags = _buffer->match<lex::StringConstant>(); !maybeFlags)
      throw RVPE(RVPE::NullaryError::Section_StringFlags, _buffer->matched_interval());
    else if (symbol) throw RVPE(RVPE::NullaryError::SymbolDeclaration_Forbidden, _buffer->matched_interval());
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
  _location_counter = nullptr;
  std::shared_ptr<pepp::tc::LinearIR> ret = nullptr;
  if (auto instr = instruction(); instr) ret = instr;
  else if (auto dot = pseudo(symbol); dot) ret = dot;
  else return nullptr;

  if (auto comment = _buffer->match<lex::InlineComment>(); comment)
    ret->insert(std::make_unique<Comment>(*comment->value));

  // Avoid re-attaching existing symbol declaration (e.g., .EQUATE in pseudo).
  if (symbol && !ret->has_attribute<SymbolDeclaration>()) ret->insert(std::make_unique<SymbolDeclaration>(*symbol));
  if (_location_counter) { // If the line declared a symbol, alias the location counter to it.
    if (symbol) _location_counter->value = std::make_shared<core::symbol::AliasValue>(4, *symbol);
    ret->insert(std::make_unique<LocationCounterDeclaration>(_location_counter));
  }
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
    // Lookahead for a comment or newline, which would indicate that this is a symbol-only line.
    // Symbol-only lines share a prefix with "normal" lines with symbols
    if (auto maybe_comment = _buffer->match<tc::lex::InlineComment>();
        symbol_decl && (_buffer->peek<tc::lex::Empty>() || maybe_comment)) {
      ret = std::make_shared<SymbolLine>(SymbolDeclaration{symbol_decl.value()});
      if (maybe_comment) ret->insert(std::make_unique<Comment>(*maybe_comment->value));
    } else ret = line(symbol_decl);

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
