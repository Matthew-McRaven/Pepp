#include "core/langs/asmb_riscv/codegen.hpp"
#include <fmt/format.h>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/formats/elf/enums.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/langs/asmb_riscv/text_format.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "ir_visitor.hpp"
#include "spdlog/spdlog.h"

pepp::tc::RISCVSectionAnalysisResults pepp::tc::riscv_split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                                                        SectionDescriptor initial_section) {
  RISCVSectionAnalysisResults ret;
  auto &grouped_ir = ret.grouped_ir;
  // Only create sections as-needed.
  decltype(&grouped_ir[0]) active = nullptr;
  // Blank / comment lines should not force a section to exist but should still be emitted in source order.
  pepp::tc::IRProgram pending;
  const auto contributes_nothing = [](const auto &line) {
    return line->type() == static_cast<int>(LinearIRType::Empty) ||
           line->type() == static_cast<int>(LinearIRType::Comment);
  };
  // Opening a section takes pending lines as its prefix.
  const auto open_section = [&](const SectionDescriptor &desc) {
    grouped_ir.emplace_back(std::make_pair(desc, std::move(pending)));
    pending.clear();
    active = &grouped_ir.back();
  };

  for (auto &line : prog) {
    if (!active && contributes_nothing(line)) {
      pending.emplace_back(line);
      continue;
    } else if (line->type() != DotSection::TYPE && !active)
      open_section(initial_section); // Code-generating lines must belong to a section.

    // TODO: Check all symbol usages are not undefined
    // TODO: .ORG for this section.
    using Type = LinearIRType;

    // Compile-time visitor pattern where the only virtual call should be type().
    switch (line->type()) {
    case DotSection::TYPE: {
      // If no existing section has the same name, create a new section with the provided flags.
      // When the section already exists, ensure that the flags match before switching to that section,
      auto as_section = std::static_pointer_cast<pepp::tc::DotSection>(line);
      auto flags = as_section->flags;
      auto name = as_section->name.value;
      auto existing_sec =
          std::find_if(grouped_ir.begin(), grouped_ir.end(), [&name](auto &i) { return i.first.name == name; });
      if (existing_sec == grouped_ir.end()) {
        pepp::tc::SectionDescriptor desc{.name = name, .flags = flags};
        // Compute the index in the ELF file which this section will become.
        desc.section_index = desc.section_base_index + ret.grouped_ir.size();
        open_section(desc);
      } else if (existing_sec->first.flags != flags) {
        throw std::logic_error("Modifying flags for an existing section");
      } else active = &*existing_sec;
      break;
    }
    case DotAlign::TYPE: {
      auto as_align = std::static_pointer_cast<pepp::tc::DotAlign>(line);
      active->first.alignment = std::max(active->first.alignment, as_align->argument.value->value_as<u16>());
      break;
    }
    case DotOrg::TYPE: {
      auto as_org = std::static_pointer_cast<pepp::tc::DotOrg>(line);
      active->first.org_count++;
      break;
    }
    default: break;
    }

    if (auto symbol_attr = line->typed_attribute<SymbolDeclaration>(); symbol_attr) {
      if (!symbol_attr->entry->is_singly_defined()) {
        auto formatted = fmt::format("Multiply defined symbol {}", symbol_attr->entry->name);
        throw std::logic_error(formatted);
      }
      // Symbols need to know their defining section to enable relocations.
      symbol_attr->entry->section_index = active->first.section_index;
    }
    if (auto counter = line->typed_attribute<LocationCounterDeclaration>(); counter)
      counter->entry->section_index = active->first.section_index;

    active->second.emplace_back(line);
  }
  // Whole file was comments, but still create a section to hold our pending lines for the sake of formatting.
  if (!pending.empty()) open_section(initial_section);
  return ret;
}
pepp::tc::IRMemoryAddressTable<pepp::tc::RISCVAddress>
pepp::tc::riscv_assign_addresses(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog, u32 initial_base_address) {
  static const auto f = [](const pepp::tc::LinearIR *line) -> pepp::core::symbol::Type {
    auto isCode = dynamic_cast<const IntegerInstruction *>(&*line);
    return isCode ? pepp::core::symbol::Type::Code : pepp::core::symbol::Type::Object;
  };
  return assign_addresses<RISCVAddress>(prog, f, initial_base_address);
}

namespace pepp::tc {
struct RISCVObjectVistitor : public RISCVIRVisitor {
  DiagnosticTable &diag;
  const IRMemoryAddressTable<RISCVAddress> &ir_to_address;
  // Mapping of addresses->this section's IR
  const LinesByAddress &lines_by_address;
  const u32 base_address;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<u8> out_bytes;
  std::vector<Relocation> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  RISCVObjectVistitor(DiagnosticTable &, const IRMemoryAddressTable<RISCVAddress> &, const LinesByAddress &,
                      const u32 base_address, bits::span<u8>, std::vector<Relocation> &, IR2ObjectCodeMap &);
  // Integer instructions can delegate to a shared implementation.
  void emit_line(const IntegerInstruction *line);
  // The value to encode for a line's operand at address, recording the relocation it needs (if any). nullopt if the
  // operand is invalid, which has been reported.
  std::optional<u32> operand_field(const LinearIR &line, pepp::ast::IRValue &value, ArgumentConstraint constraint,
                                   u32 address);
  // The value inside %pcrel_hi() at a given address. Nullopt if target is not an auipc or if it does not use
  // %pcrel_hi().
  std::optional<SymbolOffset> pcrel_hi_value(u32 address) const;
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const SymbolLine *) override;
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
  void visit(const DotConditional *) override;
  void visit(const InlineMacroDefinition *) override;
  void visit(const MacroInstantiation *) override;
};

pepp::tc::RISCVObjectVistitor::RISCVObjectVistitor(
    DiagnosticTable &diag, const IRMemoryAddressTable<RISCVAddress> &ir_to_address,
    const LinesByAddress &lines_by_address, const u32 base_address, bits::span<u8> out_bytes,
    std::vector<Relocation> &relocs, IR2ObjectCodeMap &ir_to_object_code)
    : diag(diag), ir_to_address(ir_to_address), lines_by_address(lines_by_address), base_address(base_address),
      out_bytes(out_bytes), relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::RISCVObjectVistitor::visit(const EmptyLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const CommentLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const SymbolLine *) {
  // Does not generate object code
}

namespace {
using RV = pepp::bts::RelocationsRISCV;
using K = pepp::tc::expr::Kind;
using pepp::tc::symbol_offset_patterns;
constexpr std::array call_of_symbol{K::Symbolic, K::Call};
constexpr std::array call_of_constant_plus_symbol{K::Constant, K::Symbolic, K::Add, K::Call};

// The format of the IR an operand is encoded into. Fixup rules are keyed, and sorted, by format.
enum class IRFormat : u8 { B, J, U, I, S, Word };

// How a relocated field is filled in while its symbol is defined.
enum class FixupValue : u8 {
  OperandValue,   // The operand's value, e.g. %hi(sym) or a .word.
  PcRelative,     // S + A - P, a branch or jump's offset.
  PcRelativeHigh, // The upper 20 bits of S + A - P, rounded up when the lower 12 bits will sign-extend negative.
  PcRelativeLow,  // The lower 12 bits of the PcRelativeHigh at the symbol's address, from that instruction's S + A - P.
};

struct FixupRule {
  IRFormat format;
  pepp::tc::OperandPattern operand;
  RV type;
  FixupValue value;
};

// Every (IR format, operand shape) which a relocation can express.
constexpr std::array<FixupRule, 16> fixup_rules{{
    {IRFormat::B, symbol_offset_patterns[0], RV::R_RISCV_BRANCH, FixupValue::PcRelative},
    {IRFormat::B, symbol_offset_patterns[1], RV::R_RISCV_BRANCH, FixupValue::PcRelative},
    {IRFormat::J, symbol_offset_patterns[0], RV::R_RISCV_JAL, FixupValue::PcRelative},
    {IRFormat::J, symbol_offset_patterns[1], RV::R_RISCV_JAL, FixupValue::PcRelative},
    {IRFormat::U, {.kinds = call_of_symbol, .function = "%hi"}, RV::R_RISCV_HI20, FixupValue::OperandValue},
    {IRFormat::U,
     {.kinds = call_of_constant_plus_symbol, .symbol = 1, .addend = 0, .function = "%hi"},
     RV::R_RISCV_HI20,
     FixupValue::OperandValue},
    {IRFormat::U,
     {.kinds = call_of_symbol, .function = "%pcrel_hi"},
     RV::R_RISCV_PCREL_HI20,
     FixupValue::PcRelativeHigh},
    {IRFormat::U,
     {.kinds = call_of_constant_plus_symbol, .symbol = 1, .addend = 0, .function = "%pcrel_hi"},
     RV::R_RISCV_PCREL_HI20,
     FixupValue::PcRelativeHigh},
    {IRFormat::I, {.kinds = call_of_symbol, .function = "%lo"}, RV::R_RISCV_LO12_I, FixupValue::OperandValue},
    {IRFormat::I,
     {.kinds = call_of_constant_plus_symbol, .symbol = 1, .addend = 0, .function = "%lo"},
     RV::R_RISCV_LO12_I,
     FixupValue::OperandValue},
    // %pcrel_lo names the auipc whose %pcrel_hi it completes, rather than the target.
    {IRFormat::I,
     {.kinds = call_of_symbol, .function = "%pcrel_lo"},
     RV::R_RISCV_PCREL_LO12_I,
     FixupValue::PcRelativeLow},
    {IRFormat::S, {.kinds = call_of_symbol, .function = "%lo"}, RV::R_RISCV_LO12_S, FixupValue::OperandValue},
    {IRFormat::S,
     {.kinds = call_of_constant_plus_symbol, .symbol = 1, .addend = 0, .function = "%lo"},
     RV::R_RISCV_LO12_S,
     FixupValue::OperandValue},
    {IRFormat::S,
     {.kinds = call_of_symbol, .function = "%pcrel_lo"},
     RV::R_RISCV_PCREL_LO12_S,
     FixupValue::PcRelativeLow},
    // RISC-V has no sub-word relocation for data, so only a word may hold an address.
    {IRFormat::Word, symbol_offset_patterns[0], RV::R_RISCV_32, FixupValue::OperandValue},
    {IRFormat::Word, symbol_offset_patterns[1], RV::R_RISCV_32, FixupValue::OperandValue},
}};
static_assert(std::ranges::is_sorted(fixup_rules, {}, &FixupRule::format), "Rules must be pre-sorted");

std::optional<IRFormat> format_of(const pepp::tc::LinearIR &line) {
  using Type = riscv::MnemonicDescriptor::Type;
  using Which = pepp::tc::DotLiteral::Which;
  if (const auto *instruction = dynamic_cast<const pepp::tc::IntegerInstruction *>(&line)) {
    switch (instruction->mnemonic.mn.type()) {
    case Type::B: return IRFormat::B;
    case Type::J: return IRFormat::J;
    case Type::U: return IRFormat::U;
    case Type::I: return IRFormat::I;
    case Type::S: return IRFormat::S;
    default: return std::nullopt;
    }
  } else if (const auto *literal = dynamic_cast<const pepp::tc::DotLiteral *>(&line);
             literal && literal->which == Which::Byte4)
    return IRFormat::Word;
  return std::nullopt;
}
} // namespace

std::optional<u32> pepp::tc::RISCVObjectVistitor::operand_field(const LinearIR &line, pepp::ast::IRValue &value,
                                                                ArgumentConstraint constraint, u32 address) {
  const auto format = format_of(line);
  const auto fixup = format ? find_fixup_rule<FixupRule>(fixup_rules, *format, value) : std::nullopt;
  // A constant symbol cannot be moved.
  if (fixup && !fixup->target.symbol->is_constant()) {
    const auto &[rule, target] = *fixup;
    relocations.push_back(Relocation{.target = target,
                                     .section_offset = address - base_address,
                                     .type = bits::to_underlying(rule.type),
                                     .preserve_symbol = rule.value == FixupValue::PcRelativeLow});
    if (rule.value == FixupValue::PcRelativeLow) {
      // Find the paired auipc that this line reference.
      const auto auipc = target.symbol->is_undefined() ? 0 : target.symbol->value->value()() + target.addend;
      const auto hi = target.symbol->is_undefined() ? std::nullopt : pcrel_hi_value(auipc);
      if (!hi) {
        const CodegenError error(CodegenError::NullaryError::Argument_UnpairedPcrelLo, line.source_interval);
        diag.add_message(error.loc, error.what());
        return std::nullopt;
      } else if (hi->symbol->is_undefined()) return 0;
      // Per standard, we are supposed to compute the relocation as S + A - P
      return static_cast<u32>(hi->symbol->value->value()() + hi->addend - auipc) & 0xFFF;
    } else if (rule.value != FixupValue::OperandValue) {
      // An undefined symbol is only known once linked, so no attempt is made to emit a correct immediate.
      if (target.symbol->is_undefined()) return 0;
      const u32 delta = target.symbol->value->value()() + target.addend - address;
      if (rule.value == FixupValue::PcRelativeHigh) return ((delta + 0x800) >> 12) & 0xF'FFFF;
      else if (!validate_value(diag, line, {delta, {32, expr::Signedness::Signed}}, constraint)) return std::nullopt;
      return delta;
    }
  } else if (!fixup && !symbol_offset(value) && value_needs_relocation(value)) {
    // A `symbol [+ constant]` is left to validate_argument
    const CodegenError error(CodegenError::NullaryError::Argument_NotRelocatable, line.source_interval);
    diag.add_message(error.loc, error.what());
    return std::nullopt;
  }
  if (!validate_argument(diag, line, value, constraint, 32)) return std::nullopt;
  return value.value_as<u32>();
}

std::optional<pepp::tc::SymbolOffset> pepp::tc::RISCVObjectVistitor::pcrel_hi_value(u32 address) const {
  if (const auto found = lines_by_address.find(address); found == lines_by_address.end()) return std::nullopt;
  else if (const auto *instr = dynamic_cast<const UTypeIR *>(found->second); !instr || !instr->imm) return std::nullopt;
  else if (const auto fixup = find_fixup_rule<FixupRule>(fixup_rules, IRFormat::U, *instr->imm);
           fixup && fixup->rule.type == RV::R_RISCV_PCREL_HI20)
    return fixup->target;
  return std::nullopt;
}

void pepp::tc::RISCVObjectVistitor::emit_line(const IntegerInstruction *line) {
  // Delegate to Mnemonic, which will merge pre-filled fields with provided values.
  u32 imm = line->imm ? line->imm->value_as<u32>() : u32(0);
  // An argument which cannot be encoded is reported, and encoded as 0.
  if (const auto constraint = riscv_argument_constraint(*line); line->imm && constraint)
    imm = operand_field(*line, *line->imm, *constraint, ir_to_address.at(line).address).value_or(0);
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const RTypeIR *line) { emit_line(line); }
void pepp::tc::RISCVObjectVistitor::visit(const ITypeIR *line) { emit_line(line); }
void pepp::tc::RISCVObjectVistitor::visit(const STypeIR *line) { emit_line(line); }
void pepp::tc::RISCVObjectVistitor::visit(const BTypeIR *line) { emit_line(line); }
void pepp::tc::RISCVObjectVistitor::visit(const UTypeIR *line) { emit_line(line); }
void pepp::tc::RISCVObjectVistitor::visit(const JTypeIR *line) { emit_line(line); }

void pepp::tc::RISCVObjectVistitor::visit(const DotAlign *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotLiteral *line) {
  auto addr_info = ir_to_address.at(line);
  auto &value = *line->argument.value;
  if (const auto constraint = riscv_argument_constraint(*line);
      constraint && !operand_field(*line, value, *constraint, addr_info.address)) {
    std::ranges::fill(out_bytes.first(addr_info.size), 0);
  } else {
    // RV32 is little-endian.
    (void)value.serialize(out_bytes.first(addr_info.size), bits::Order::LittleEndian);
  }

  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotBlock *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotEquate *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const DotSection *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const DotOrg *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const DotConditional *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const InlineMacroDefinition *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const MacroInstantiation *) {
  // Flattened before generating object code
}

} // namespace pepp::tc
std::optional<pepp::tc::ArgumentConstraint> pepp::tc::riscv_argument_constraint(const LinearIR &line) {
  using Which = DotLiteral::Which;
  using Destination = riscv::Operand::Destination;
  using enum expr::Signedness;
  if (const auto *literal = dynamic_cast<const DotLiteral *>(&line)) {
    switch (literal->which) {
    case Which::ASCII: return std::nullopt;
    case Which::Byte1: return ArgumentConstraint{8, Bits};
    case Which::Byte2: return ArgumentConstraint{16, Bits};
    case Which::Byte4: return ArgumentConstraint{32, Bits};
    }
  } else if (const auto *instruction = dynamic_cast<const IntegerInstruction *>(&line)) {
    const auto &desc = instruction->mnemonic.mn;
    // Fence orderings are decoded by the parser rather than written as a value.
    if (desc.width_imm() == 0 || desc.sources(Destination::PRED) || desc.sources(Destination::SUCC))
      return std::nullopt;
    else if (desc.sources(Destination::SHAMT)) return ArgumentConstraint{5, Unsigned};
    // `lui rd, 0x65` writes the argument as given, while branch and jump targets drop their implicit low bit.
    else if (desc.type() == riscv::MnemonicDescriptor::Type::U) return ArgumentConstraint{desc.width_imm(), Unsigned};
    const u8 shift = desc.imm_shift();
    return ArgumentConstraint{static_cast<u8>(desc.width_imm() + shift), Signed, static_cast<u8>(1 << shift)};
  }
  return std::nullopt;
}

pepp::tc::ProgramObjectCodeResult
pepp::tc::riscv_to_object_code(DiagnosticTable &diag, const IRMemoryAddressTable<RISCVAddress> &addresses,
                               std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  return to_object_code<RISCVAddress, RISCVObjectVistitor>(diag, addresses, prog);
}

pepp::tc::ElfResult pepp::tc::riscv_to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                           const IRMemoryAddressTable<RISCVAddress> &addrs,
                                           const ProgramObjectCodeResult &object_code,
                                           const pepp::core::symbol::LeafTable &symbols) {
  using namespace pepp::bts;
  SPDLOG_INFO("Creating RISC-V ELF");
  auto ret = sections_to_elf(ElfBits::b32, ElfEndian::le, ElfMachineType::EM_RISCV, prog, object_code, symbols);

  ret.ir_to_listing = number_listing_lines(prog, object_code, riscv_listing_bytes_per_line);
  // RV32 addresses are 4 bytes.
  const auto r_riscv_32 = bits::to_underlying(RelocationsRISCV::R_RISCV_32);
  write_debug_line(ret.elf, 4, r_riscv_32, r_riscv_32, prog, addrs, ret.ir_to_listing);

  /*ELFIO::section *symTab = nullptr;
  for (auto &sec : ret->sections)
    if (sec->get_type() == ELFIO::SHT_SYMTAB && sec->get_name() == "os.symtab") symTab = &*sec;
  Q_ASSERT(symTab != nullptr);
  // TODO: populate mmios if mmios is not empty
  obj::addMMIONoteSection(*ret);
  ::obj::addMMIODeclarations(*ret, symTab, mmios);*/
  // pas::obj::common::writeLineMapping(*_elf, *_osRoot);
  //  pas::obj::common::writeDebugCommands(*_elf, {&*_osRoot});
  return ret;
}
