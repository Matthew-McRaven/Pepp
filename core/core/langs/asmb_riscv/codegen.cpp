#include "core/langs/asmb_riscv/codegen.hpp"
#include <fmt/format.h>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
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
  const IRMemoryAddressTable<RISCVAddress> &ir_to_address;
  const u32 base_address, section_idx;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<u8> out_bytes;
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  RISCVObjectVistitor(const IRMemoryAddressTable<RISCVAddress> &, const u32 base_address, const u16 section_idx,
                      bits::span<u8>, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &,
                      IR2ObjectCodeMap &);
  // Integer instructions can delegate to a shared implementation.
  void emit_line(const IntegerInstruction *line);
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

pepp::tc::RISCVObjectVistitor::RISCVObjectVistitor(
    const IRMemoryAddressTable<RISCVAddress> &ir_to_address, const u32 base_address, const u16 section_idx,
    bits::span<u8> out_bytes, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocs,
    IR2ObjectCodeMap &ir_to_object_code)
    : ir_to_address(ir_to_address), base_address(base_address), section_idx(section_idx), out_bytes(out_bytes),
      relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::RISCVObjectVistitor::visit(const EmptyLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const CommentLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::emit_line(const IntegerInstruction *line) {
  // Delegate to Mnemonic, which will merge pre-filled fields with provided values.
  const u32 imm = line->imm ? line->imm->value_as<u32>() : u32(0);
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
  // Emit relocations for undefined symbolic arguments.
  auto as_symbolic_arg = std::dynamic_pointer_cast<pepp::ast::Symbolic>(line->argument.value);
  if (as_symbolic_arg != nullptr) {
    auto symbol = as_symbolic_arg->symbol();
    if (symbol->is_undefined()) {
      u16 offset = addr_info.address - base_address;
      relocations.insert({symbol, StaticRelocation{.section_offset = offset, .section_idx = section_idx}});
    }
  }
  // RV32 is little-endian.
  (void)line->argument.value->serialize(out_bytes.first(addr_info.size), bits::Order::LittleEndian);

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

} // namespace pepp::tc
pepp::tc::ProgramObjectCodeResult
pepp::tc::riscv_to_object_code(const IRMemoryAddressTable<RISCVAddress> &addresses,
                               std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  return to_object_code<RISCVAddress, RISCVObjectVistitor>(addresses, prog);
}

pepp::tc::ElfResult pepp::tc::riscv_to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                           const IRMemoryAddressTable<RISCVAddress> &addrs,
                                           const ProgramObjectCodeResult &object_code) {
  using namespace pepp::bts;
  SPDLOG_INFO("Creating RISC-V ELF");
  auto ret = sections_to_elf(ElfBits::b32, ElfEndian::le, ElfMachineType::EM_RISCV, prog, object_code);

  // TODO: restore once .debug_line can be written to a packed file
  // ret.ir_to_listing = write_line_mapping(*ret.elf, prog, addrs, object_code);

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
