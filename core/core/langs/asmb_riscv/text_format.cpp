#include "core/langs/asmb_riscv/text_format.hpp"
#include <fmt/format.h>
#include <optional>
#include <string>
#include <vector>
#include "core/arch/riscv/isa/rv_base.hpp"
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/langs/asmb_riscv/ir_visitor.hpp"
#include "core/math/bitmanip/strings.hpp"

std::string pepp::tc::riscv_format_as_columns(const std::string &col0, const std::string &col1,
                                              const std::string &col2, const std::string &col3) {
  using O = RISCVFormatOptions;
  const auto formatted =
      fmt::format("{:<{}}{:<{}}{:<{}}{}", col0, O::col0_width,
                  col1.size() >= O::col1_width ? col1 + " " : col1, O::col1_width, col2, O::col2_width, col3);
  return bits::rtrimmed(formatted);
}

namespace {
using namespace pepp::tc;

// RISC-V comments run to end of line after '#', per the lexer's line_comment_leader.
std::string comment_of(const LinearIR *line) {
  if (auto maybe = line->typed_attribute<Comment>(); maybe) return "#" + maybe->value;
  return "";
}
std::string symbol_of(const LinearIR *line) {
  if (auto maybe = line->typed_attribute<SymbolDeclaration>(); maybe) return std::string{maybe->entry->name} + ":";
  return "";
}
std::string imm_of(const IntegerInstruction *line) { return line->imm ? line->imm->string() : std::string{}; }
// FENCE keeps its two orderings packed into the immediate and needs to access the integer value.
u32 imm_value_of(const IntegerInstruction *line) { return line->imm ? line->imm->value_as<u32>() : 0; }

// Architectural names (x0..x31) rather than ABI names.
// TODO: remember which of the two the user actually wrote.
std::string_view reg(u8 n) { return riscv::xname(n); }

// Respect descriptor's declared order rather than assuming it is fixed.
std::string operands_of(const IntegerInstruction *line) {
  using D = riscv::Operand::Destination;
  using OT = riscv::Operand::Type;
  const auto &desc = line->mnemonic.mn;
  const auto ops = desc.operands();
  std::string out;
  for (std::size_t i = 0; i < ops.size(); ++i) {
    std::string text;
    switch (ops[i].destination) {
    case D::RD: text = reg(line->rd); break;
    // RS is the lone source of a two-operand pseudo, which the parser stores as rs1.
    case D::RS: [[fallthrough]];
    case D::RS1: text = reg(line->rs1); break;
    case D::RS2: text = reg(line->rs2); break;
    case D::IMM: [[fallthrough]];
    case D::SHAMT: text = imm_of(line); break;
    // A fence packs both orderings into the immediate: pred at imm[7:4], succ at imm[3:0].
    case D::PRED: text = riscv::fence_ordering_name((imm_value_of(line) >> 4) & 0xF); break;
    case D::SUCC: text = riscv::fence_ordering_name(imm_value_of(line) & 0xF); break;
    case D::Invalid: continue;
    }
    // If any op needs parens, surround it now.
    out += ops[i].type == OT::ParenthesizedRegister ? fmt::format("({})", text) : text;
    // Insert trailing separator if needed.
    if (i + 1 < ops.size() && desc.comma_after(i)) out += ", ";
  }
  return out;
}

std::string format_instruction(const IntegerInstruction *line) {
  return riscv_format_as_columns(symbol_of(line), std::string{line->mnemonic.name}, operands_of(line),
                                 comment_of(line));
}
} // namespace

namespace pepp::tc {
struct RISCVSourceVisitor : public RISCVIRVisitor {
  std::string text;
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const RTypeIR *) override;
  void visit(const ITypeIR *) override;
  void visit(const STypeIR *) override;
  void visit(const BTypeIR *) override;
  void visit(const UTypeIR *) override;
  void visit(const JTypeIR *) override;
  void visit(const DotAlign *) override;
  void visit(const DotLiteral *) override;
  void visit(const DotBlock *) override;
  void visit(const DotEquate *) override;
  void visit(const DotSection *) override;
  void visit(const DotOrg *) override;
};
} // namespace pepp::tc

void pepp::tc::RISCVSourceVisitor::visit(const EmptyLine *) { text = ""; }

void pepp::tc::RISCVSourceVisitor::visit(const CommentLine *line) {
  text = riscv_format_as_columns("#" + line->comment.value, "", "", "");
}

// Delegate to the MnemonicDescriptor-based formatting.
void pepp::tc::RISCVSourceVisitor::visit(const RTypeIR *line) { text = ::format_instruction(line); }
void pepp::tc::RISCVSourceVisitor::visit(const ITypeIR *line) { text = ::format_instruction(line); }
void pepp::tc::RISCVSourceVisitor::visit(const STypeIR *line) { text = ::format_instruction(line); }
void pepp::tc::RISCVSourceVisitor::visit(const BTypeIR *line) { text = ::format_instruction(line); }
void pepp::tc::RISCVSourceVisitor::visit(const UTypeIR *line) { text = ::format_instruction(line); }
void pepp::tc::RISCVSourceVisitor::visit(const JTypeIR *line) { text = ::format_instruction(line); }

void pepp::tc::RISCVSourceVisitor::visit(const DotAlign *line) {
  // RISC-V spells the two alignment forms separately rather than inferring from the argument.
  const auto dot = line->which == DotAlign::Which::Pow2 ? ".p2align" : ".balign";
  text = riscv_format_as_columns(::symbol_of(line), dot, line->argument.value->string(), ::comment_of(line));
}

void pepp::tc::RISCVSourceVisitor::visit(const DotLiteral *line) {
  std::string dot = "";
  using Which = DotLiteral::Which;
  switch (line->which) {
  case Which::ASCII: dot = ".ascii"; break;
  case Which::Byte1: dot = ".byte"; break;
  case Which::Byte2: dot = ".half"; break;
  case Which::Byte4: dot = ".word"; break;
  }
  text = riscv_format_as_columns(::symbol_of(line), dot, line->argument.value->string(), ::comment_of(line));
}

void pepp::tc::RISCVSourceVisitor::visit(const DotBlock *line) {
  text = riscv_format_as_columns(::symbol_of(line), ".skip", line->argument.value->string(), ::comment_of(line));
}

void pepp::tc::RISCVSourceVisitor::visit(const DotEquate *line) {
  text = riscv_format_as_columns(std::string{line->symbol.entry->name} + ":", ".equ",
                                 line->argument.value->string(), ::comment_of(line));
}

void pepp::tc::RISCVSourceVisitor::visit(const DotSection *line) {
  const auto args = fmt::format("\"{}\", \"{}\"", line->name.value, line->flags.to_string());
  text = riscv_format_as_columns("", ".section", args, ::comment_of(line));
}

void pepp::tc::RISCVSourceVisitor::visit(const DotOrg *line) {
  const auto dot = line->behavior == DotOrg::Behavior::BURN ? ".burn" : ".org";
  text = riscv_format_as_columns("", dot, line->argument.value->string(), ::comment_of(line));
}

std::string pepp::tc::riscv_format_source(const LinearIR *line) {
  RISCVSourceVisitor v;
  accept(v, line);
  return v.text;
}

std::vector<std::string> pepp::tc::riscv_format_source(const IRProgram &program) {
  std::vector<std::string> ret;
  for (const auto &line : program) ret.emplace_back(riscv_format_source(&*line));
  return ret;
}

namespace {
void riscv_format_listing(const LinearIR *line, const IRMemoryAddressTable<RISCVAddress> &addresses,
                          const ProgramObjectCodeResult &object_code, std::vector<std::string> &out) {
  const auto source_line = pepp::tc::riscv_format_source(line);
  const auto address_it = addresses.find(line);
  const auto address =
      address_it == addresses.cend() ? std::nullopt : std::optional<u32>(address_it->second.address);

  const std::string address_str = address.has_value() ? fmt::format("{:08X}", *address) : std::string(8, ' ');

  const auto code_it = object_code.ir_to_object_code.find(line);
  bits::span<u8> code = code_it == object_code.ir_to_object_code.end() ? bits::span<u8>{} : code_it->second;

  static constexpr int bytes_per_line = 4;
  auto obj = std::vector<char>(2 * bytes_per_line);
  auto n = std::min<size_t>(bytes_per_line, code.size());
  auto end = bits::bytesToAsciiHex(obj, code.first(n), {});
  code = code.subspan(n);

  const auto initial_line = fmt::format("{:<8} {:<8} {}", address_str, std::string_view(obj.data(), end), source_line);
  out.emplace_back(initial_line);
  while (!code.empty()) {
    n = std::min<size_t>(bytes_per_line, code.size());
    end = bits::bytesToAsciiHex(obj, code.first(n), {});
    out.emplace_back(fmt::format("{:<8} {}", "", std::string_view(obj.data(), end)));
    code = code.subspan(n);
  }
}
} // namespace

std::vector<std::string> pepp::tc::riscv_format_listing(const LinearIR *line,
                                                  const IRMemoryAddressTable<RISCVAddress> &addresses,
                                                  const ProgramObjectCodeResult &object_code) {
  std::vector<std::string> ret;
  ::riscv_format_listing(line, addresses, object_code, ret);
  return ret;
}

std::vector<std::string> pepp::tc::riscv_format_listing(const IRProgram &program,
                                                  const IRMemoryAddressTable<RISCVAddress> &addresses,
                                                  const ProgramObjectCodeResult &object_code) {
  std::vector<std::string> ret;
  for (const auto &line : program) ::riscv_format_listing(&*line, addresses, object_code, ret);
  return ret;
}
