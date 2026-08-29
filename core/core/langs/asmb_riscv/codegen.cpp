#include "core/langs/asmb_riscv/codegen.hpp"
#include <elfio/elfio.hpp>
#include <fmt/format.h>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb/elfio_utils.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "ir_visitor.hpp"
#include "spdlog/spdlog.h"

pepp::tc::RISCVSectionAnalysisResults pepp::tc::riscv_split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                                                        SectionDescriptor initial_section) {
  RISCVSectionAnalysisResults ret;
  ret.grouped_ir.emplace_back(std::make_pair(initial_section, pepp::tc::IRProgram{}));
  auto &grouped_ir = ret.grouped_ir;
  auto *active = &grouped_ir[0];
  for (auto &line : prog) {
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
        grouped_ir.emplace_back(std::make_pair(desc, pepp::tc::IRProgram{}));
        active = &grouped_ir.back();
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

void pepp::tc::RISCVObjectVistitor::visit(const RTypeIR *line) {
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = line->rd, .imm = std::nullopt};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const ITypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const STypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = std::nullopt, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const BTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = std::nullopt, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const UTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = std::nullopt, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const JTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = std::nullopt, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

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
  (void)line->argument.value->serialize(out_bytes.first(addr_info.size), bits::Order::BigEndian);

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

static std::shared_ptr<ELFIO::elfio> create_elf() {
  SPDLOG_INFO("Creating pep/10 ELF");
  u16 mac = 243; // EM_RISCV (0xF3)
  auto ret = std::make_shared<ELFIO::elfio>();
  ret->create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2LSB);
  ret->set_os_abi(ELFIO::ELFOSABI_NONE);
  ret->set_type(ELFIO::ET_EXEC);
  ret->set_machine(mac);
  // Create strtab/notes early, so that it will be before any code sections.
  pepp::tc::addStrTab(*ret);
  return ret;
}

pepp::tc::ElfResult pepp::tc::riscv_to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                           const IRMemoryAddressTable<RISCVAddress> &addrs,
                                           const ProgramObjectCodeResult &object_code) {
  ELFIO::segment *activeSeg = nullptr;
  ElfResult ret;
  ret.elf = create_elf();
  ret.section_offsets.resize(prog.size(), 0);

  auto getOrCreateBSS = [&](SectionFlags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() != 0) {
      activeSeg = ret.elf->segments.add();
      activeSeg->set_type(ELFIO::PT_LOAD);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.r ? ELFIO::PF_R : 0;
      elfFlags |= flags.w ? ELFIO::PF_W : 0;
      elfFlags |= flags.x ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  auto getOrCreateBits = [&](SectionFlags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() == 0 ||
        !(((activeSeg->get_flags() & ELFIO::PF_R) > 0 == flags.r) &&
          ((activeSeg->get_flags() & ELFIO::PF_W) > 0 == flags.w) &&
          ((activeSeg->get_flags() & ELFIO::PF_X) > 0 == flags.x))) {
      activeSeg = ret.elf->segments.add();
      activeSeg->set_type(ELFIO::PT_LOAD);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.r ? ELFIO::PF_R : 0;
      elfFlags |= flags.w ? ELFIO::PF_W : 0;
      elfFlags |= flags.x ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  u32 skipped_sections = 0;
  std::vector<size_t> section_memory_sizes(prog.size(), 0);
  for (u32 it = 0; it < prog.size(); it++) {
    auto &sec = prog[it].first;
    section_memory_sizes[it] = sec.high_address - sec.low_address;
  }
  for (u32 it = 0; it < prog.size(); it++) {
    ret.section_offsets[it] = skipped_sections;
    if (section_memory_sizes[it] == 0) { // 0-sized sections are meaningless, do not emit.
      skipped_sections++;
      continue;
    }

    auto &sec_desc = prog[it].first;

    SPDLOG_INFO("{} creating", sec_desc.name);

    auto sec = ret.elf->sections.add(sec_desc.name);
    if (sec->get_index() != (sec_desc.section_index - skipped_sections))
      throw std::logic_error("Mismatch in pre-computed section index");
    // All sections from AST correspond to bits in Pep/10 memory, so alloc
    auto shFlags = ELFIO::SHF_ALLOC;
    shFlags |= sec_desc.flags.x ? ELFIO::SHF_EXECINSTR : 0;
    shFlags |= sec_desc.flags.w ? ELFIO::SHF_WRITE : 0;
    sec->set_flags(shFlags);
    sec->set_addr_align(sec_desc.alignment);
    SPDLOG_TRACE("{} sized at {:x}", sec_desc.name, section_memory_sizes[it]);

    if (sec_desc.flags.z) {
      SPDLOG_TRACE("{} zeroed", sec_desc.name);
      sec->set_type(ELFIO::SHT_NOBITS);
      sec->set_size(section_memory_sizes[it]);
    } else {
      auto sec_data = object_code.section_spans[it];
      SPDLOG_TRACE("{} assigned {:x} bytes", sec_desc.name, sec_data.object_code.size());
      // Cannot convert between quint8 and qint8 without reinterpret cast. Sorry for future linter errors.
      sec->set_data(reinterpret_cast<char *>(sec_data.object_code.data()), sec_data.object_code.size_bytes());
      sec->set_type(ELFIO::SHT_PROGBITS);
    }

    if (sec_desc.flags.z) getOrCreateBSS(sec_desc.flags);
    else getOrCreateBits(sec_desc.flags);

    activeSeg->add_section(sec, sec_desc.alignment);
    activeSeg->set_physical_address(
        std::min<ELFIO::Elf64_Addr>(activeSeg->get_physical_address(), sec_desc.low_address));
    activeSeg->set_virtual_address(std::min<ELFIO::Elf64_Addr>(activeSeg->get_virtual_address(), sec_desc.low_address));
    SPDLOG_TRACE("{} base address set to {:x}", sec_desc.name, sec_desc.low_address);

    // Field not re-computed on its own. Failure to compute will cause readelf to crash.
    // TODO: in the future, handle alignment correctly?
    // if (isOS) activeSeg->set_memory_size(activeSeg->get_memory_size() + size);
  }

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
