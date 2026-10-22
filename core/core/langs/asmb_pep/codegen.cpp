#include "core/langs/asmb_pep/codegen.hpp"
#include <list>
#include <numeric>
#include <ranges>
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/enums.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/langs/asmb_pep/text_format.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "zpp_bits.h"

namespace {
using PepRelocation = pepp::bts::RelocationsPep;
using pepp::tc::symbol_offset_patterns;

enum class IRFormat : u8 { OperandSpecifier, Byte, Word };

struct FixupRule {
  IRFormat format;
  pepp::tc::OperandPattern operand;
  PepRelocation type;
};

// Every (IR format, operand pattern) which needs to be relocated in Pep/10
constexpr std::array<FixupRule, 6> fixup_rules{{
    {IRFormat::OperandSpecifier, symbol_offset_patterns[0], PepRelocation::R_PEP10_ABS16},
    {IRFormat::OperandSpecifier, symbol_offset_patterns[1], PepRelocation::R_PEP10_ABS16},
    // R_PEP10_ABS8 truncates an address to a byte.
    {IRFormat::Byte, symbol_offset_patterns[0], PepRelocation::R_PEP10_ABS8},
    {IRFormat::Byte, symbol_offset_patterns[1], PepRelocation::R_PEP10_ABS8},
    {IRFormat::Word, symbol_offset_patterns[0], PepRelocation::R_PEP10_ABS16},
    {IRFormat::Word, symbol_offset_patterns[1], PepRelocation::R_PEP10_ABS16},
}};
static_assert(std::ranges::is_sorted(fixup_rules, {}, &FixupRule::format), "Rules must be pre-sorted");

std::optional<IRFormat> format_of(const pepp::tc::LinearIR &line) {
  using Which = pepp::tc::DotLiteral::Which;
  if (line.type() == pepp::tc::DyadicInstruction::TYPE) return IRFormat::OperandSpecifier;
  else if (const auto *literal = dynamic_cast<const pepp::tc::DotLiteral *>(&line)) {
    if (literal->which == Which::Byte1) return IRFormat::Byte;
    else if (literal->which == Which::Byte2 || literal->which == Which::Address2) return IRFormat::Word;
  }
  return std::nullopt;
}

// The fixup rule for a line's operand
std::optional<pepp::tc::MatchedFixupRule<FixupRule>> find_fixup(const pepp::tc::LinearIR &line,
                                                                pepp::ast::IRValue &value) {
  const auto format = format_of(line);
  return format ? pepp::tc::find_fixup_rule<FixupRule>(fixup_rules, *format, value) : std::nullopt;
}
} // namespace

pepp::tc::PeppSectionAnalysisResults pepp::tc::pepp_split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                                                      SectionDescriptor initial_section) {
  PeppSectionAnalysisResults ret;
  auto &grouped_ir = ret.grouped_ir;
  // Only create sections as-needed.
  decltype(&grouped_ir[0]) active = nullptr;
  // Blank / comment lines should not force a section to exist but should still be emitted in source order.
  pepp::tc::IRProgram pending;
  const auto contributes_nothing = [](const auto &line) {
    return line->type() == static_cast<int>(LinearIRType::Empty) ||
           line->type() == static_cast<int>(LinearIRType::Comment);
  };
  // A symbol which can move must be used in a form which a fixup rule can relocate.
  const auto check_operand = [&diag](const LinearIR &line, pepp::ast::IRValue &arg) {
    if (value_needs_relocation(arg) && !find_fixup(line, arg))
      diag.add_message(line.source_interval, "Operands referencing symbols must be of the form symbol [+ constant]",
                       Severity::Error);
  };
  // Stack adjustments the debugger cannot follow statically.
  const auto check_sp_increment = [&diag](const DyadicInstruction &line) {
    const auto name = line.mnemonic.name;
    if ((name == "ADDSP" || name == "SUBSP") && line.addr_mode.addr_mode != pep::AddrMode::I)
      diag.add_message(line.source_interval,
                       fmt::format("Stack trace not possible unless immediate addressing is specified", name),
                       Severity::Warning);
  };
  int burn_count = 0;
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
      } else {
        if (existing_sec->first.flags != flags) {
          const auto message = CodegenError::to_string(CodegenError::UnaryError::Section_FlagsChanged, name);
          diag.add_message(line->source_interval, message, Severity::Fatal);
        }
        // Always continue processing the current section to maximimize the number of errors that can be reported in one
        // pass.
        active = &*existing_sec;
      }
      break;
    }
    case DotAlign::TYPE: {
      auto as_align = std::static_pointer_cast<pepp::tc::DotAlign>(line);
      active->first.alignment = std::max(active->first.alignment, as_align->argument.value->value_as<u16>());
      break;
    }
    case DotAnnotate::TYPE: {
      auto as_annotate = std::static_pointer_cast<pepp::tc::DotAnnotate>(line);
      if (as_annotate->which == DotAnnotate::Which::END) break;
      auto arg_str = as_annotate->argument.value->string();
      if (as_annotate->which == DotAnnotate::Which::SCALL) ret.system_calls.emplace_back(arg_str);
      else if (as_annotate->which == DotAnnotate::Which::INPUT)
        ret.mmios.emplace_back(IO{.name = arg_str, .type = IO::Type::kInput});
      else if (as_annotate->which == DotAnnotate::Which::OUTPUT)
        ret.mmios.emplace_back(IO{.name = arg_str, .type = IO::Type::kOutput});
      break;
    }
    case DotOrg::TYPE: {
      auto as_org = std::static_pointer_cast<pepp::tc::DotOrg>(line);
      if (as_org->behavior == DotOrg::Behavior::ORG) {
        if (active->first.org_count++ > 0)
          diag.add_message(line->source_interval, "Only one .ORG is allowed per section", Severity::Fatal);
      } else if (burn_count++ > 0) {
        diag.add_message(line->source_interval, "Only one .BURN is allowed", Severity::Fatal);
      }
      break;
    }
    case DyadicInstruction::TYPE: {
      const auto &instr = *static_cast<DyadicInstruction *>(line.get());
      check_operand(*line, *instr.argument.value);
      check_sp_increment(instr);
      break;
    }
    case DotLiteral::TYPE: check_operand(*line, *static_cast<DotLiteral *>(line.get())->argument.value); break;
    case InlineMacroDefinition::TYPE: [[fallthrough]];
    case MacroInstantiation::TYPE: throw std::logic_error("Cannot perform code generation on macros");

    default: break;
    }

    if (auto symbol_attr = line->typed_attribute<SymbolDeclaration>(); symbol_attr) {
      if (!symbol_attr->entry->is_singly_defined()) {
        const auto symbol = std::string(symbol_attr->entry->name);
        const auto message = CodegenError::to_string(CodegenError::UnaryError::Symbol_MultiplyDefined, symbol);
        diag.add_message(line->source_interval, message, Severity::Fatal);
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
  suppress_zeroed_object_code(grouped_ir);
  suppress_object_code_before_burn(prog);
  return ret;
}

pepp::tc::IRMemoryAddressTable<pepp::tc::PeppAddress>
pepp::tc::pepp_assign_addresses(DiagnosticTable &diag, std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                u16 initial_base_address) {
  static const auto f = [](const pepp::tc::LinearIR *line) -> pepp::core::symbol::Type {
    auto isCode = dynamic_cast<const DyadicInstruction *>(&*line) || dynamic_cast<const MonadicInstruction *>(&*line);
    return isCode ? pepp::core::symbol::Type::Code : pepp::core::symbol::Type::Object;
  };
  return assign_addresses<PeppAddress>(diag, prog, f, initial_base_address);
}

namespace pepp::tc {
struct PeppObjectVistitor : public PepIRVisitor {
  DiagnosticTable &diag;
  const IRMemoryAddressTable<PeppAddress> &ir_to_address;
  const u16 base_address;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<u8> out_bytes;
  std::vector<Relocation> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  // Lines by address are unused, since Pep/10 has no relocation which refers to another line.
  PeppObjectVistitor(DiagnosticTable &, const IRMemoryAddressTable<PeppAddress> &, const LinesByAddress &,
                     const u16 base_address, bits::span<u8>, std::vector<Relocation> &, IR2ObjectCodeMap &);
  // Record a relocation for the argument at offset into the section. False if it can't be relocated.
  bool relocate(const LinearIR &line, pepp::ast::IRValue &arg, u16 offset);
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const SymbolLine *) override;
  void visit(const MonadicInstruction *) override;
  void visit(const DyadicInstruction *) override;
  void visit(const DotAlign *) override;
  void visit(const DotLiteral *) override;
  void visit(const DotBlock *) override;
  void visit(const DotEquate *) override;
  void visit(const DotSection *) override;
  void visit(const DotAnnotate *) override;
  void visit(const DotOrg *) override;
  void visit(const DotConditional *) override;
  void visit(const InlineMacroDefinition *) override;
  void visit(const MacroInstantiation *) override;
};

pepp::tc::PeppObjectVistitor::PeppObjectVistitor(
    DiagnosticTable &diag, const IRMemoryAddressTable<PeppAddress> &ir_to_address, const LinesByAddress &,
    const u16 base_address, bits::span<u8> out_bytes, std::vector<Relocation> &relocs,
    IR2ObjectCodeMap &ir_to_object_code)
    : diag(diag), ir_to_address(ir_to_address), base_address(base_address), out_bytes(out_bytes), relocations(relocs),
      ir_to_object_code(ir_to_object_code) {}

namespace {
// The linker fills in undefined symbols, and addresses of code/data may shift. Constants cannot move.
bool needs_relocation(const pepp::core::symbol::Entry &symbol) {
  using enum pepp::core::symbol::Type;
  const auto &value = symbol.value;
  return symbol.is_undefined() || value == nullptr || value->type() == Object || value->type() == Code ||
         value->type() == LocationCounter;
}
} // namespace

bool pepp::tc::PeppObjectVistitor::relocate(const LinearIR &line, pepp::ast::IRValue &arg, u16 offset) {
  const auto fixup = find_fixup(line, arg);
  // This should be a previously-reported error, but allow the object code to be generated without crashing.
  if (!fixup && value_needs_relocation(arg)) return false;
  else if (!fixup || !needs_relocation(*fixup->target.symbol)) return true;
  relocations.push_back(
      Relocation{.target = fixup->target, .section_offset = offset, .type = bits::to_underlying(fixup->rule.type)});
  return true;
}

void pepp::tc::PeppObjectVistitor::visit(const EmptyLine *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const CommentLine *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const SymbolLine *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const MonadicInstruction *line) {
  out_bytes[0] = line->mnemonic.mn.encode(line->mnemonic.mn.default_addr);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(1)});
  out_bytes = out_bytes.subspan(1);
}

void pepp::tc::PeppObjectVistitor::visit(const DyadicInstruction *line) {
  auto addr_info = ir_to_address.at(line);
  out_bytes[0] = line->mnemonic.mn.encode(line->addr_mode.addr_mode);
  const u16 offset = addr_info.address - base_address + 1; // Offset by 1 to reach operand specifier.
  auto &value = *line->argument.value;
  if (validate_argument(diag, *line, value, *pepp_argument_constraint(*line), 16, true) &&
      relocate(*line, value, offset))
    (void)value.serialize(out_bytes.subspan(1).first(2), bits::Order::BigEndian);
  else std::ranges::fill(out_bytes.subspan(1).first(2), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(3)});
  out_bytes = out_bytes.subspan(3);
}

void pepp::tc::PeppObjectVistitor::visit(const DotAlign *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::PeppObjectVistitor::visit(const DotLiteral *line) {
  auto addr_info = ir_to_address.at(line);
  auto &value = *line->argument.value;
  // R_PEP10_ABS8 truncates an address to a byte, so every Pep argument may hold an address.
  const auto constraint = pepp_argument_constraint(*line);
  if ((!constraint || validate_argument(diag, *line, value, *constraint, 16, true)) &&
      relocate(*line, value, addr_info.address - base_address))
    (void)value.serialize(out_bytes.first(addr_info.size), bits::Order::BigEndian);
  else std::ranges::fill(out_bytes.first(addr_info.size), 0);

  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::PeppObjectVistitor::visit(const DotBlock *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::PeppObjectVistitor::visit(const DotEquate *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const DotSection *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const DotAnnotate *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const DotOrg *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const DotConditional *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const InlineMacroDefinition *) {
  // Does not generate object code
}

void pepp::tc::PeppObjectVistitor::visit(const MacroInstantiation *) {
  // Does not generate object code
}

std::optional<ArgumentConstraint> pepp_argument_constraint(const LinearIR &line) {
  using Which = DotLiteral::Which;
  using enum expr::Signedness;
  if (const auto *literal = dynamic_cast<const DotLiteral *>(&line)) {
    switch (literal->which) {
    case Which::ASCII: return std::nullopt;
    case Which::Byte1: return ArgumentConstraint{8, Bits};
    case Which::Byte2: [[fallthrough]];
    case Which::Address2: return ArgumentConstraint{16, Bits};
    case Which::Byte4: return ArgumentConstraint{32, Bits};
    }
  } else if (line.type() == DyadicInstruction::TYPE) return ArgumentConstraint{16, Bits};
  return std::nullopt;
}

ProgramObjectCodeResult pepp_to_object_code(DiagnosticTable &diag, const IRMemoryAddressTable<PeppAddress> &addresses,
                                            std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  // Every section shares the 16-bit address space, so their sizes are summed, NOBITS included.
  u64 total = 0;
  for (const auto &[desc, ir] : prog) total += desc.byte_count;
  if (total > pep_address_space_size) {
    // No one line is at fault, so the error covers the whole file.
    using support::Location;
    diag.add_message({Location(0, 0), Location(Location::MAX, Location::MAX)},
                     fmt::format("Object code must fit within {} bytes.", pep_address_space_size), Severity::Fatal);
    return {};
  }
  return to_object_code<PeppAddress, PeppObjectVistitor>(diag, addresses, prog);
}

} // namespace pepp::tc

pepp::tc::ElfResult pepp::tc::pepp_to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                          const IRMemoryAddressTable<PeppAddress> &addrs,
                                          const ProgramObjectCodeResult &object_code,
                                          const pepp::core::symbol::LeafTable &symbols,
                                          const std::vector<IO> &mmios, const DebugLineFiles &files,
                                          const std::vector<std::shared_ptr<const InlineMacroDefinition>> &macros,
                                          pepp::bts::ElfMachineType machine, const IR2ListingLineMap *listing) {
  using namespace pepp::bts;
  SPDLOG_INFO("Creating pep ELF");
  auto ret = sections_to_elf(ElfBits::b32, ElfEndian::be, machine, prog, object_code, symbols);

  // Pep/10 addresses are 2 bytes.
  const auto abs16 = bits::to_underlying(RelocationsPep::R_PEP10_ABS16);
  const auto abs32 = bits::to_underlying(RelocationsPep::R_PEP10_ABS32);
  write_debug_line(ret.elf, 2, abs16, abs32, prog, addrs, listing, files, macros);

  if (!mmios.empty()) {
    auto f = [&](auto &file) {
      if (!file) return;
      MMIONoteWriter writer(*file, find_named_section(*file, ".symtab"));
      for (const auto &io : mmios)
        if (!writer.add(io)) throw std::logic_error("pepp_to_elf: no symbol for MMIO port " + io.name);
    };
    std::visit(f, ret.elf);
  }
  return ret;
}
