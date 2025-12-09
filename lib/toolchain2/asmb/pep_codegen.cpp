#include "./pep_codegen.hpp"
#include <elfio/elfio.hpp>
#include <ranges>
#include "bits/copy.hpp"
#include "fmt/format.h"
#include "mmio.hpp"
#include "pep_ir_visitor.hpp"
#include "spdlog/spdlog.h"
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/visit.hpp"
#include "toolchain2/asmb/common_elf.hpp"

pepp::tc::SectionAnalysisResults pepp::tc::split_to_sections(PepIRProgram &prog, SectionDescriptor initial_section) {
  SectionAnalysisResults ret;
  ret.grouped_ir.emplace_back(std::make_pair(initial_section, pepp::tc::PepIRProgram{}));
  auto &grouped_ir = ret.grouped_ir;
  auto *active = &grouped_ir[0];
  for (auto &line : prog) {
    // TODO: Check all symbol usages are not undefined
    // TODO: .BURN for this section.

    using Type = ir::LinearIR::Type;

    // Compile-time visitor pattern where the only virtual call should be type().
    switch (line->type()) {
    case Type::DotSection: {
      // If no existing section has the same name, create a new section with the provided flags.
      // When the section already exists, ensure that the flags match before switching to that section,
      auto as_section = std::static_pointer_cast<pepp::tc::ir::DotSection>(line);
      auto flags = as_section->flags;
      auto name = as_section->name.to_string();
      auto existing_sec =
          std::find_if(grouped_ir.begin(), grouped_ir.end(), [&name](auto &i) { return i.first.name == name; });
      if (existing_sec == grouped_ir.end()) {
        pepp::tc::SectionDescriptor desc{.name = name.toStdString(), .flags = flags};
        // Compute the index in the ELF file which this section will become.
        desc.section_index = desc.section_base_index + ret.grouped_ir.size();
        grouped_ir.emplace_back(std::make_pair(desc, pepp::tc::PepIRProgram{}));
        active = &grouped_ir.back();
      } else if (existing_sec->first.flags != flags) {
        throw std::logic_error("Modifying flags for an existing section");
      } else active = &*existing_sec;
      break;
    }
    case Type::DotAlign: {
      auto as_align = std::static_pointer_cast<pepp::tc::ir::DotAlign>(line);
      active->first.alignment = std::max(active->first.alignment, as_align->argument.value->value<quint16>());
      break;
    }
    case Type::DotAnnotate: {
      auto as_annotate = std::static_pointer_cast<pepp::tc::ir::DotAnnotate>(line);
      auto arg_str = as_annotate->argument.value->string();
      if (as_annotate->which == ir::DotAnnotate::Which::SCALL) ret.system_calls.emplace_back(arg_str.toStdString());
      else if (as_annotate->which == ir::DotAnnotate::Which::INPUT)
        ret.mmios.emplace_back(obj::IO{.name = arg_str, .type = obj::IO::Type::kInput});
      else if (as_annotate->which == ir::DotAnnotate::Which::OUTPUT)
        ret.mmios.emplace_back(obj::IO{.name = arg_str, .type = obj::IO::Type::kOutput});
      break;
    }
    case Type::DotOrg: {
      auto as_org = std::static_pointer_cast<pepp::tc::ir::DotOrg>(line);
      active->first.org_count++;
      break;
    }
    default: break;
    }

    if (auto symbol_attr = line->typed_attribute<ir::attr::SymbolDeclaration>(); symbol_attr) {
      if (!symbol_attr->entry->is_singly_defined()) {
        auto formatted = fmt::format("Multiply defined symbol {}", symbol_attr->entry->name.toStdString());
        throw std::logic_error(formatted);
      }
      // Symbols need to know their defining section to enable relocations.
      symbol_attr->entry->section_index = active->first.section_index;
    }

    active->second.emplace_back(line);
  }
  return ret;
}

struct SectionAddrInfo {
  int index, previous;
};

pepp::tc::IRMemoryAddressTable pepp::tc::assign_addresses(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog,
                                                          quint16 initial_base_address) {
  enum class Direction { Forward, Backward } direction = Direction::Forward;

  // Pre-allocate vector according to the total size of the IR lines in all sections.
  // This overrserves storage---not all IR lines generate object code---but is a stable upper bound and avoid
  // reallocation in the address-assignment loop.
  size_t size = 0;
  for (const auto &sec : prog) size += sec.second.size();
  // ITEMS ARE NOT INSERTED IN SORTED ORDER. DO NOT USE AS A MAP UNTIL SORTING.
  // Since all IR lines across all PepIRProgram have unique (C++) addresses, we can blindly append and sort later.
  // This gives O(1) insert rather than  n* O(nlgn) with the requirement for a manual sort before returning.
  IRMemoryAddressTable ret;
  ret.container.reserve(size);

  // Process all sections affected by one .ORG before processing any sections affected by the next one.
  // There may be some IR lines in an .ORG section that are before the .ORG and iterating left-to-right lets us assign
  // addresses to those items correctly.
  std::vector<SectionAddrInfo> sorted_work;
  sorted_work.reserve(prog.size());

  // Phase 1: Determine the order in which sections should be assigned addresses.
  // This is generally left-to-right, unless there are sections before the first .ORG or the program contains a .BURN,
  // in which case some subset of sections is assigned right-to-left.

  // Sections before the first .ORG need to be grouped right rather than left
  size_t first_org_section = -1;
  // Record all sections which contain at least one .ORG
  for (int it = 0; it < prog.size(); it++) {
    if (prog[it].first.org_count > 0) {
      sorted_work.emplace_back(SectionAddrInfo{it, it});
      // If this is the first .ORG, we need to insert the the first 0..it sections
      if (first_org_section == -1) {
        first_org_section = it;
        for (int jt = it - 1; jt >= 0; jt--) sorted_work.emplace_back(SectionAddrInfo{jt, jt + 1});
      }
    } else if (first_org_section != -1) sorted_work.emplace_back(SectionAddrInfo{it, it - 1});
  }

  // Program contained no .ORGs. Act as if first section begins with a .ORG <initial_base_address>.
  if (first_org_section == -1) {
    sorted_work.insert(sorted_work.begin(), SectionAddrInfo{0, 0});
    for (int it = 1; it < prog.size(); it++) sorted_work.emplace_back(SectionAddrInfo{it, it - 1});
  }

  if (prog.size() != sorted_work.size()) throw std::logic_error("Layout of sections failed");

  // Phase 2: assign addresses for each section to each line of IR.
  quint16 base_address = 0;

  // Using templates to type-erase the difference between forward and reverse iterators
  auto for_lines = [&](auto &&range, SectionDescriptor &sec_desc) {
    for (auto &line : range) {
      auto maybe_size = line->object_size(base_address);
      quint16 symbol_base = base_address, next_base = base_address, size = maybe_size.value_or(0);

      // Perform special handling for non-code-generating dot commands
      using Type = ir::LinearIR::Type;
      switch (line->type()) {
      case Type::DotOrg:
        base_address = std::static_pointer_cast<ir::DotOrg>(line)->argument.value->template value<quint16>();
        symbol_base = next_base = base_address;
        break;
      case Type::DotEquate: {
        auto as_equate = std::static_pointer_cast<ir::DotEquate>(line);
        auto symbol = as_equate->symbol.entry;
        auto argument = as_equate->argument.value;
        // Re-use from previous assembler
        if (auto symbolic = dynamic_cast<pas::ast::value::Symbolic *>(&*argument); symbolic != nullptr) {
          auto other = symbolic->symbol();
          if (symbol::rootTable(other->parent.sharedFromThis()) == symbol::rootTable(symbol->parent.sharedFromThis())) {
            symbol->value = QSharedPointer<symbol::value::InternalPointer>::create(sizeof(quint16), other);
          } else {
            symbol->value = QSharedPointer<symbol::value::ExternalPointer>::create(
                sizeof(quint16), other->parent.sharedFromThis(), other);
          }
        } else {
          auto bits = symbol::value::MaskedBits{.byteCount = 2, .bitPattern = 0, .mask = 0xFFFF};
          argument->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&bits.bitPattern), 8}, bits::hostOrder());
          symbol->value = QSharedPointer<symbol::value::Constant>::create(bits);
        }
        continue; // Must resume loop early, or symbol will be clobbered below.
      }
      default: break;
      }

      if (!maybe_size.has_value()) continue;
      else if (direction == Direction::Forward) {
        // Must explicitly handle address wrap-around, because math inside set
        // address widens implicitly.
        next_base = (base_address + size) % 0x10000;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        ret.container.emplace_back(line.get(), ir::attr::Address(base_address, size));
        base_address = next_base;
      } else {
        next_base = (base_address - size) % 0x10000;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        auto adjustedAddress = next_base + (size > 0 ? 1 : 0);
        // If we use newBase, we are off-by-one when size is non-zero.
        symbol_base = adjustedAddress;
        ret.container.emplace_back(line.get(), ir::attr::Address(adjustedAddress % 0x10000, size));
        base_address = next_base;
      }
      sec_desc.byte_count += size;

      if (auto line_symbol = line->template typed_attribute<ir::attr::SymbolDeclaration>(); line_symbol) {
        auto isCode = dynamic_cast<ir::DyadicInstruction *>(&*line) || dynamic_cast<ir::MonadicInstruction *>(&*line);
        line_symbol->entry->value = QSharedPointer<symbol::value::Location>::create(
            size, sizeof(quint16), symbol_base, 0, isCode ? symbol::Type::kCode : symbol::Type::kObject);
      }
    }
  };

  for (auto &sec_idx : sorted_work) {
    auto &sec = prog[sec_idx.index];

    // If this section contains the first org, there may be some lines BEFORE that org which need to be assigned
    // backwards.
    if (sec_idx.index == first_org_section) {
      // Split the program into two ranges: the region before the ORG and the rest.
      auto it = sec.second.begin();
      for (; it != sec.second.end(); it++)
        if ((*it)->type() == ir::LinearIR::Type::DotOrg) break;

      auto org_arg = static_pointer_cast<ir::DotOrg>(*it)->argument.value->value<quint16>();
      // Find index of first ORG and assign BACKWARD from there, exluding the ORG. Set section's low_address.
      base_address = org_arg - 1, direction = Direction::Backward;
      for_lines(std::views::reverse(std::ranges::subrange(sec.second.begin(), it)), sec.first);
      sec.first.low_address = base_address;

      // Assign rest of section (including ORG) FORWARD. Set section's high_address.
      base_address = org_arg, direction = Direction::Forward;
      for_lines(std::views::all(std::ranges::subrange(it, sec.second.end())), sec.first);
      sec.first.high_address = base_address;
      continue; // Skip normal address computations.
    }

    // Determine base address and direction for the section.
    if (sec_idx.index == sec_idx.previous) {
      // Program did not contain any ORGs, use address parameter.
      if (sec_idx.index == 0) base_address = initial_base_address;
      else base_address = prog[sec_idx.index - 1].first.high_address;
      direction = Direction::Forward;
    } else if (sec_idx.index > sec_idx.previous) {
      direction = Direction::Forward;
      base_address = prog[sec_idx.previous].first.high_address;
    } else {
      direction = Direction::Backward;
      base_address = prog[sec_idx.previous].first.low_address;
    }

    if (direction == Direction::Forward) {
      sec.first.low_address = base_address;
      for_lines(std::views::all(sec.second), sec.first);
      sec.first.high_address = base_address;
    } else {
      sec.first.high_address = base_address;
      for_lines(std::views::reverse(sec.second), sec.first);
      sec.first.low_address = base_address;
    }
  }

  // Establish flat_map invariant, which is that the container is sorted.
  std::sort(ret.container.begin(), ret.container.end(), detail::IRComparator{});
  return ret;
}

namespace pepp::tc {
struct ObjectCodeVisitor : public ir::LinearIRVisitor {
  const IRMemoryAddressTable &ir_to_address;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<quint8> out_bytes;
  std::vector<void *> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  ObjectCodeVisitor(const IRMemoryAddressTable &, bits::span<quint8>, std::vector<void *> &, IR2ObjectCodeMap &);
  void visit(const ir::EmptyLine *) override;
  void visit(const ir::CommentLine *) override;
  void visit(const ir::MonadicInstruction *) override;
  void visit(const ir::DyadicInstruction *) override;
  void visit(const ir::DotAlign *) override;
  void visit(const ir::DotLiteral *) override;
  void visit(const ir::DotBlock *) override;
  void visit(const ir::DotEquate *) override;
  void visit(const ir::DotSection *) override;
  void visit(const ir::DotAnnotate *) override;
  void visit(const ir::DotOrg *) override;
};

pepp::tc::ObjectCodeVisitor::ObjectCodeVisitor(const IRMemoryAddressTable &ir_to_address, bits::span<quint8> out_bytes,
                                               std::vector<void *> &relocs, IR2ObjectCodeMap &ir_to_object_code)
    : ir_to_address(ir_to_address), out_bytes(out_bytes), relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::EmptyLine *) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::CommentLine *) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::MonadicInstruction *line) {
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(1)});
  out_bytes = out_bytes.subspan(1);
}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DyadicInstruction *line) {
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction, line->addr_mode.addr_mode);
  line->argument.value->value(out_bytes.subspan(1).first(2), bits::Order::BigEndian);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(3)});
  out_bytes = out_bytes.subspan(3);
}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotAlign *line) {
  auto addr_info = ir_to_address.at(static_cast<const ir::DotAlign *const>(line));
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotLiteral *line) {
  auto addr_info = ir_to_address.at(static_cast<const ir::DotLiteral *const>(line));
  line->argument.value->value(out_bytes.first(addr_info.size), bits::Order::BigEndian);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotBlock *line) {
  auto addr_info = ir_to_address.at(static_cast<const ir::DotBlock *const>(line));
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotEquate *) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotSection *) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotAnnotate *) {}

void pepp::tc::ObjectCodeVisitor::visit(const ir::DotOrg *) {}

} // namespace pepp::tc

static QSharedPointer<ELFIO::elfio> create_elf() {
  SPDLOG_INFO("Creating pep/10 ELF");
  static const char p10mac[2] = {'p', 'x'};
  quint16 mac;
  bits::memcpy_endian({(quint8 *)&mac, 2}, bits::hostOrder(), {(const quint8 *)p10mac, 2}, bits::Order::BigEndian);
  auto ret = QSharedPointer<ELFIO::elfio>::create();
  ret->create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2MSB);
  ret->set_os_abi(ELFIO::ELFOSABI_NONE);
  ret->set_type(ELFIO::ET_EXEC);
  ret->set_machine(mac);
  // Create strtab/notes early, so that it will be before any code sections.
  pepp::tc::addStrTab(*ret);
  return ret;
}
pepp::tc::ElfResult pepp::tc::to_elf(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog,
                                     const IRMemoryAddressTable &addrs, const ProgramObjectCodeResult &object_code,
                                     const std::vector<obj::IO> &mmios) {

  ELFIO::segment *activeSeg = nullptr;
  ElfResult ret;
  ret.elf = create_elf();
  ret.section_offsets.resize(prog.size(), 0);

  auto getOrCreateBSS = [&](ir::attr::SectionFlags &flags) {
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

  auto getOrCreateBits = [&](ir::attr::SectionFlags &flags) {
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

  quint32 skipped_sections = 0;
  std::vector<size_t> section_memory_sizes(prog.size(), 0);
  for (int it = 0; it < prog.size(); it++) {
    auto &sec = prog[it].first;
    section_memory_sizes[it] = sec.high_address - sec.low_address;
  }
  for (int it = 0; it < prog.size(); it++) {
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
      SPDLOG_TRACE("{} zeroed", fullName);
      sec->set_type(ELFIO::SHT_NOBITS);
      sec->set_size(section_memory_sizes[it]);
    } else {
      SPDLOG_TRACE("{} assigned {:x} bytes", fullName, bytes.size());
      auto sec_data = object_code.section_spans[it];
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

namespace {
struct SectionOffsets {
  size_t object_code_offset = 0, object_code_size = 0;
  size_t reloc_offset = 0, reloc_size = 0;
};
} // namespace
pepp::tc::ProgramObjectCodeResult
pepp::tc::to_object_code(const IRMemoryAddressTable &addresses,
                         std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog) {
  ProgramObjectCodeResult ret;
  using Item = std::pair<SectionDescriptor, PepIRProgram>;
  std::vector<SectionOffsets> offsets(prog.size(), SectionOffsets{});
  quint32 object_size = 0, ir_count = 0;
  for (int it = 0; it < prog.size(); it++) {
    const auto &sec = prog[it];
    if (sec.first.flags.z) continue; // No bytes in ELF for Z section; no relocations possible.
    offsets[it].object_code_size = sec.first.byte_count;
    offsets[it].object_code_offset = object_size;
    object_size += sec.first.byte_count;
    ir_count += sec.second.size();
  }
  ret.object_code.resize(object_size, 0);
  ret.ir_to_object_code.container.reserve(ir_count);
  ret.section_spans.reserve(prog.size());

  for (int it = 0; it < prog.size(); it++) {
    const auto &sec = prog[it];
    auto &offset = offsets[it];
    auto code_begin = ret.object_code.begin() + offset.object_code_offset;
    auto code_end = code_begin + offset.object_code_size;

    auto oc_subspan = bits::span<quint8>(code_begin, code_end);
    ObjectCodeVisitor visitor(addresses, oc_subspan, ret.relocations, ret.ir_to_object_code);
    offset.reloc_offset = ret.relocations.size();
    for (const auto &line : sec.second) line->accept(&visitor);
    offset.reloc_size = offset.reloc_offset - ret.relocations.size();
  }

  // SectionInfo cannot be created until core loop is complete, because relocation might re-allocate and invalidate
  // relocation info.
  using SectionSpans = ProgramObjectCodeResult::SectionSpans;
  for (int it = 0; it < prog.size(); it++) {
    auto &offset = offsets[it];
    const auto &sec = prog[it];
    // Z sections need entries in section_spans, but those entries should be empty.
    if (sec.first.flags.z) {
      ret.section_spans.emplace_back(SectionSpans{{}, {}});
    } else {
      auto code_begin = ret.object_code.begin() + offset.object_code_offset;
      auto code_end = code_begin + offset.object_code_size;
      auto oc_subspan = bits::span<quint8>(code_begin, code_end);
      auto reloc_begin = ret.relocations.begin() + offset.reloc_offset;
      auto reloc_end = reloc_begin + offset.reloc_size;
      auto reloc_subspan = bits::span<void *>(reloc_begin, reloc_end);
      ret.section_spans.emplace_back(SectionSpans{oc_subspan, reloc_subspan});
    }
  }

  //  Establish flat-map invariant
  std::sort(ret.ir_to_object_code.container.begin(), ret.ir_to_object_code.container.end(), IR2ObjectComparator{});
  return ret;
}

void pepp::tc::write_symbol_table(ElfResult &elf_wrapper, symbol::Table &symbol_table, const QString name) {
  using namespace Qt::StringLiterals;
  auto &elf = *elf_wrapper.elf.get();
  auto strTab = pepp::tc::addStrTab(elf);
  auto symTab = elf.sections.add(u"%1"_s.arg(name).toStdString());
  symTab->set_type(ELFIO::SHT_SYMTAB);
  symTab->set_info(0);
  symTab->set_addr_align(2);
  symTab->set_entry_size(elf.get_default_entry_size(ELFIO::SHT_SYMTAB));
  symTab->set_link(strTab->get_index());

  // Attempt to pool strings when possible, to reduce final binary size.
  // Probably O(n^2), but n should be small for Pep/N.
  // TODO: Would like to find a way to reuse my existing StringPool here.
  ELFIO::string_section_accessor strAc(strTab);
  auto findOrCreateStr = [&](const std::string &str) {
    auto tabStart = strTab->get_data();
    auto tabEnd = tabStart + strTab->get_size();
    // Must use data/size+1 and not begin/end because we MUST include trailing null.
    // Otherwise, `main` is pooled with `mainCln`, which is wrong.
    auto iter = std::search(tabStart, tabEnd, str.data(), str.data() + str.size() + 1);
    if (iter != tabEnd) return (ELFIO::Elf_Word)(iter - tabStart);
    return strAc.add_string(str.data());
  };

  ELFIO::symbol_section_accessor symAc(elf, symTab);
  for (auto [name, entry] : symbol_table.entries()) {
    auto nameIdx = findOrCreateStr(name.toStdString());
    // Our sections inserted into ELF do not start at 0, they start at SectionDescriptor::section_base_index.
    // section_offsets starts at 0, so we need to convert before indexing or risk an out-of-bounds access.
    auto offset_index = entry->section_index - SectionDescriptor::section_base_index;
    // But sections before SectionDescriptor::section_base_index are special and never skipped.
    auto section_index_adjustment =
        entry->section_index < SectionDescriptor::section_base_index ? elf_wrapper.section_offsets[offset_index] : 0;
    auto secIdx = entry->section_index - section_index_adjustment;
    auto value = entry->value;

    quint8 type = ELFIO::STT_NOTYPE;
    if (value->type() == symbol::Type::kCode) type = ELFIO::STT_FUNC;
    else if (value->type() == symbol::Type::kObject) type = ELFIO::STT_OBJECT;
    else if (value->type() == symbol::Type::kConstant) {
      type = ELFIO::STT_OBJECT;
      secIdx = ELFIO::SHN_ABS;
    }

    quint8 bind = ELFIO::STB_LOCAL;
    if (entry->binding == symbol::Binding::kGlobal) bind = ELFIO::STB_GLOBAL;
    else if (entry->binding == symbol::Binding::kImported) bind = ELFIO::STB_WEAK;

    quint8 info = (bind << 4) + (type & 0xf);

    if (entry->state == symbol::DefinitionState::kUndefined) secIdx = ELFIO::SHN_UNDEF;
    symAc.add_symbol(nameIdx, value->value()(), entry->value->size(), info, 0,
                     secIdx); // leave other as 0, don't mess with visibility.
  }
  // To be elf compliant, local symbols must be before all other kinds.
  symAc.arrange_local_symbols();
}
