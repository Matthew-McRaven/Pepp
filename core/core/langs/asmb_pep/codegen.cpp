#include "core/langs/asmb_pep/codegen.hpp"
#include <core/compile/ir_linear/line_comment.hpp>
#include <elfio/elfio.hpp>
#include <list>
#include <numeric>
#include <ranges>
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/elfio_utils.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "zpp_bits.h"

pepp::tc::SectionAnalysisResults pepp::tc::split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                                             SectionDescriptor initial_section) {
  SectionAnalysisResults ret;
  ret.grouped_ir.emplace_back(std::make_pair(initial_section, pepp::tc::IRProgram{}));
  auto &grouped_ir = ret.grouped_ir;
  auto *active = &grouped_ir[0];
  for (auto &line : prog) {
    // TODO: Check all symbol usages are not undefined
    // TODO: .BURN for this section.

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
    case DotAnnotate::TYPE: {
      auto as_annotate = std::static_pointer_cast<pepp::tc::DotAnnotate>(line);
      auto arg_str = as_annotate->argument.value->string();
      if (as_annotate->which == DotAnnotate::Which::SCALL) ret.system_calls.emplace_back(arg_str);
      else if (as_annotate->which == DotAnnotate::Which::INPUT)
        ret.mmios.emplace_back(obj::IO{.name = arg_str, .type = obj::IO::Type::kInput});
      else if (as_annotate->which == DotAnnotate::Which::OUTPUT)
        ret.mmios.emplace_back(obj::IO{.name = arg_str, .type = obj::IO::Type::kOutput});
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

struct SectionAddrInfo {
  int index, previous;
};

pepp::tc::IRMemoryAddressTable<pepp::tc::PeppAddress>
pepp::tc::assign_addresses(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog, u16 initial_base_address) {
  enum class Direction { Forward, Backward } direction = Direction::Forward;

  // Pre-allocate vector according to the total size of the IR lines in all sections.
  // This overrserves storage---not all IR lines generate object code---but is a stable upper bound and avoid
  // reallocation in the address-assignment loop.
  i64 size = 0;
  for (const auto &[desc, ir] : prog) size += ir.size();
  // ITEMS ARE NOT INSERTED IN SORTED ORDER. DO NOT USE AS A MAP UNTIL SORTING.
  // Since all IR lines across all PepIRProgram have unique (C++) addresses, we can blindly append and sort later.
  // This gives O(1) insert rather than  n* O(nlgn) with the requirement for a manual sort before returning.
  IRMemoryAddressTable<pepp::tc::PeppAddress> ret;
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
  i64 first_org_section = -1;
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
  u16 base_address = 0;

  // Using templates to type-erase the difference between forward and reverse iterators
  auto for_lines = [&](auto &&range, SectionDescriptor &sec_desc) {
    for (auto &line : range) {
      auto maybe_size = line->object_size(base_address);
      u16 symbol_base = base_address, next_base = base_address, size = maybe_size.value_or(0);

      // Perform special handling for non-code-generating dot commands
      using Type = LinearIRType;
      switch (line->type()) {
      case (int)Type::DotOrg:
        base_address = std::static_pointer_cast<DotOrg>(line)->argument.value->template value_as<u16>();
        symbol_base = next_base = base_address;
        break;
      case (int)Type::DotEquate: {
        auto as_equate = std::static_pointer_cast<DotEquate>(line);
        auto symbol = as_equate->symbol.entry;
        auto argument = as_equate->argument.value;
        // Re-use from previous assembler
        if (auto symbolic = dynamic_cast<pepp::ast::Symbolic *>(&*argument); symbolic != nullptr) {
          auto other = symbolic->symbol();
          symbol->value = std::make_shared<pepp::core::symbol::AliasValue>(2, other);
        } else {
          auto masked_bits = bits::MaskedBits{.byteCount = 2, .bitPattern = 0, .mask = 0xFFFF};
          (void)argument->serialize(bits::span<u8>{reinterpret_cast<u8 *>(&masked_bits.bitPattern), 2},
                                    bits::hostOrder());
          symbol->value = std::make_shared<pepp::core::symbol::ConstantValue>(masked_bits);
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
        ret.container.emplace_back(line.get(), PeppAddress(base_address, size));
        base_address = next_base;
      } else {
        next_base = (base_address - size) % 0x10000;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        auto adjustedAddress = next_base + (size > 0 ? 1 : 0);
        // If we use newBase, we are off-by-one when size is non-zero.
        symbol_base = adjustedAddress;
        ret.container.emplace_back(line.get(), PeppAddress(adjustedAddress % 0x10000, size));
        base_address = next_base;
      }
      sec_desc.byte_count += size;

      if (auto line_symbol = line->template typed_attribute<SymbolDeclaration>(); line_symbol) {
        auto isCode = dynamic_cast<DyadicInstruction *>(&*line) || dynamic_cast<MonadicInstruction *>(&*line);
        const auto type = isCode ? pepp::core::symbol::Type::Code : pepp::core::symbol::Type::Object;
        line_symbol->entry->value =
            std::make_shared<pepp::core::symbol::LocationValue>(size, sizeof(u16), symbol_base, 0, type);
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
        if ((*it)->type() == DotOrg::TYPE) break;

      auto org_arg = static_pointer_cast<DotOrg>(*it)->argument.value->value_as<u16>();
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
  std::sort(ret.container.begin(), ret.container.end(), detail::IRComparator<PeppAddress>{});
  return ret;
}

namespace pepp::tc {
struct ObjectCodeVisitor : public PepIRVisitor {
  const IRMemoryAddressTable<PeppAddress> &ir_to_address;
  const u16 base_address, section_idx;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<u8> out_bytes;
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  ObjectCodeVisitor(const IRMemoryAddressTable<PeppAddress> &, const u16 base_address, const u16 section_idx,
                    bits::span<u8>, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &,
                    IR2ObjectCodeMap &);
  void visit(const EmptyLine *);
  void visit(const CommentLine *);
  void visit(const MonadicInstruction *);
  void visit(const DyadicInstruction *);
  void visit(const DotAlign *);
  void visit(const DotLiteral *);
  void visit(const DotBlock *);
  void visit(const DotEquate *);
  void visit(const DotSection *);
  void visit(const DotAnnotate *);
  void visit(const DotOrg *);
};

pepp::tc::ObjectCodeVisitor::ObjectCodeVisitor(
    const IRMemoryAddressTable<PeppAddress> &ir_to_address, const u16 base_address, const u16 section_idx,
    bits::span<u8> out_bytes, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocs,
    IR2ObjectCodeMap &ir_to_object_code)
    : ir_to_address(ir_to_address), base_address(base_address), section_idx(section_idx), out_bytes(out_bytes),
      relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::ObjectCodeVisitor::visit(const EmptyLine *) {
  // Does not generate object code
}

void pepp::tc::ObjectCodeVisitor::visit(const CommentLine *) {
  // Does not generate object code
}

void pepp::tc::ObjectCodeVisitor::visit(const MonadicInstruction *line) {
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(1)});
  out_bytes = out_bytes.subspan(1);
}

void pepp::tc::ObjectCodeVisitor::visit(const DyadicInstruction *line) {
  auto addr_info = ir_to_address.at(line);
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction, line->addr_mode.addr_mode);
  // Emit relocations for undefined symbolic arguments.
  auto as_symbolic_arg = std::dynamic_pointer_cast<pepp::ast::Symbolic>(line->argument.value);
  if (as_symbolic_arg != nullptr) {
    auto symbol = as_symbolic_arg->symbol();
    if (symbol->is_undefined()) {
      u16 offset = addr_info.address - base_address;
      relocations.insert({symbol, StaticRelocation{.section_offset = offset, .section_idx = section_idx}});
    }
  }
  (void)line->argument.value->serialize(out_bytes.subspan(1).first(2), bits::Order::BigEndian);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(3)});
  out_bytes = out_bytes.subspan(3);
}

void pepp::tc::ObjectCodeVisitor::visit(const DotAlign *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ObjectCodeVisitor::visit(const DotLiteral *line) {
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

void pepp::tc::ObjectCodeVisitor::visit(const DotBlock *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ObjectCodeVisitor::visit(const DotEquate *) {
  // Does not generate object code
}

void pepp::tc::ObjectCodeVisitor::visit(const DotSection *) {
  // Does not generate object code
}

void pepp::tc::ObjectCodeVisitor::visit(const DotAnnotate *) {
  // Does not generate object code
}

void pepp::tc::ObjectCodeVisitor::visit(const DotOrg *) {
  // Does not generate object code
}

ELFIO::section *getLineMappingSection(ELFIO::elfio &elf) {
  ELFIO::section *lineNumbers = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == lineMapStr && sec->get_type() == ELFIO::SHT_PROGBITS) {
      lineNumbers = sec.get();
      break;
    }
  }
  return lineNumbers;
}

} // namespace pepp::tc

namespace {
struct SectionOffsets {
  size_t object_code_offset = 0, object_code_size = 0;
  size_t reloc_offset = 0, reloc_size = 0;
};
} // namespace
pepp::tc::ProgramObjectCodeResult pepp::tc::to_object_code(const IRMemoryAddressTable<PeppAddress> &addresses,
                                                           std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  ProgramObjectCodeResult ret;
  std::vector<SectionOffsets> offsets(prog.size(), SectionOffsets{});
  u32 object_size = 0, ir_count = 0;
  for (u32 it = 0; it < prog.size(); it++) {
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

  for (u32 it = 0; it < prog.size(); it++) {
    const auto &[sec, ir] = prog[it];
    auto &offset = offsets[it];
    auto code_begin = ret.object_code.begin() + offset.object_code_offset;
    auto code_end = code_begin + offset.object_code_size;

    auto oc_subspan = bits::span<u8>(code_begin, code_end);
    ObjectCodeVisitor visitor(addresses, sec.low_address, it, oc_subspan, ret.relocations, ret.ir_to_object_code);
    offset.reloc_offset = ret.relocations.size();
    for (const auto &line : ir) accept(visitor, line.get());
    offset.reloc_size = offset.reloc_offset - ret.relocations.size();
  }

  // SectionInfo cannot be created until core loop is complete, because relocation might re-allocate and invalidate
  // relocation info.
  using SectionSpans = ProgramObjectCodeResult::SectionSpans;
  for (u32 it = 0; it < prog.size(); it++) {
    const auto &[desc, ir] = prog[it];
    auto &offset = offsets[it];
    // Z sections need entries in section_spans, but those entries should be empty.
    if (desc.flags.z) {
      ret.section_spans.emplace_back(SectionSpans{{}});
    } else {
      auto code_begin = ret.object_code.begin() + offset.object_code_offset;
      auto code_end = code_begin + offset.object_code_size;
      ret.section_spans.emplace_back(SectionSpans{bits::span<u8>(code_begin, code_end)});
    }
  }

  //  Establish flat-map invariant
  std::sort(ret.ir_to_object_code.container.begin(), ret.ir_to_object_code.container.end(), IR2ObjectComparator{});
  return ret;
}

static std::shared_ptr<ELFIO::elfio> create_elf() {
  SPDLOG_INFO("Creating pep/10 ELF");
  static const char p10mac[2] = {'p', 'x'};
  u16 mac;
  bits::memcpy_endian({(u8 *)&mac, 2}, bits::hostOrder(), {(const u8 *)p10mac, 2}, bits::Order::BigEndian);
  auto ret = std::make_shared<ELFIO::elfio>();
  ret->create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2MSB);
  ret->set_os_abi(ELFIO::ELFOSABI_NONE);
  ret->set_type(ELFIO::ET_EXEC);
  ret->set_machine(mac);
  // Create strtab/notes early, so that it will be before any code sections.
  pepp::tc::addStrTab(*ret);
  return ret;
}

pepp::tc::IR2ListingLineMap
write_line_mapping(ELFIO::elfio &elf,
                   const std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>> &prog,
                   const pepp::tc::IRMemoryAddressTable<pepp::tc::PeppAddress> &addrs,
                   const pepp::tc::ProgramObjectCodeResult &object_code) {
  auto line_section = pepp::tc::getLineMappingSection(elf);
  if (line_section == nullptr) {
    line_section = elf.sections.add(pepp::tc::lineMapStr);
    line_section->set_type(ELFIO::SHT_PROGBITS);
  }

  // Compute the the listing line for each IR in the re-arranged source program.
  // Assumes 3 bytes of object code per listing line.
  pepp::tc::IR2ListingLineMap ret;
  ret.container.reserve(
      std::accumulate(prog.cbegin(), prog.cend(), 0, [](size_t l, auto &r) { return l + r.second.size(); }));
  u16 listing_number = 0;
  for (const auto &sec : prog) {
    for (const auto &line : sec.second) {
      auto oc_it = object_code.ir_to_object_code.find(line.get());
      if (oc_it == object_code.ir_to_object_code.cend())
        ret.container.emplace_back(pepp::tc::IR2ListingLinePair{line.get(), listing_number++});
      else {
        ret.container.emplace_back(pepp::tc::IR2ListingLinePair{line.get(), listing_number});
        listing_number += 1 + oc_it->second.size() / 3;
      }
    }
  }
  std::sort(ret.container.begin(), ret.container.end(), pepp::tc::IR2ListingLineComparator{});

  auto [data, in, out] = zpp::bits::data_in_out();
  pepp::tc::BinaryLineMapping prev;
  bool first = true;
  for (const auto &sec : prog) {
    for (const auto &line : sec.second) {
      auto addr_it = addrs.find(line.get());
      // Row numbers are 0-indexed, while binary line mappings are 1-indexed.
      uint16_t src_line = line->source_interval.valid() ? 1 + line->source_interval.lower().row : 0;
      auto lst_it = ret.find(line.get());
      uint16_t lst_line = lst_it != ret.end() ? 1 + lst_it->second : 0;

      // addr2line makes no sense without an address or a line number.
      if (src_line == 0 && lst_line == 0) continue;
      else if (addr_it == addrs.cend()) continue;

      auto current =
          pepp::tc::BinaryLineMapping{.address = addr_it->second.address, .srcLine = src_line, .listLine = lst_line};
      (void)current.serialize(out, current, &prev, first);
      prev = current, first = false;
    }
  }
  line_section->append_data((const char *)data.data(), out.position());
  return ret;
}

pepp::tc::ElfResult pepp::tc::to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                     const IRMemoryAddressTable<PeppAddress> &addrs,
                                     const ProgramObjectCodeResult &object_code, const std::vector<obj::IO> &mmios) {

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

  ret.ir_to_listing = write_line_mapping(*ret.elf, prog, addrs, object_code);

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

