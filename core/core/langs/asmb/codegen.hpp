#pragma once

#include <list>
#include <map>
#include <numeric>
#include <ranges>
#include <vector>
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/ir_program.hpp"

namespace pepp::tc {
namespace detail {
struct SectionAddrInfo {
  int index, previous;
};

struct SectionOffsets {
  size_t object_code_offset = 0, object_code_size = 0;
  size_t reloc_offset = 0, reloc_size = 0;
};
} // namespace detail

using LineToSymbolType = pepp::core::symbol::Type(const pepp::tc::LinearIR *);

struct SectionDescriptor {
  std::string name;
  pepp::tc::SectionFlags flags;
  u16 alignment = 1, org_count = 0;
  std::optional<u16> base_address = std::nullopt;
  u32 low_address = 0, high_address = 0;
  // Number of bytes that will be written out to ELF file.
  // Not high_address-low_address because .ORG can mess with address!
  u32 byte_count = 0;
  // Symbols need to know which section they are going to be relocated against.
  // Rather than wait until the elf file has been generated, we can verify (through source code inspection!) the number
  // that will be assigned to the first non-ELF-plumbing section.
  // Then, splitting to sections can increment this counter AND update the symbol declaration's links.
  static constexpr u16 section_base_index = 3;
  u16 section_index = section_base_index;
};

// assign_addresses iterates over sections from prog, grouping non-ORG sections contiguously with the nearest ORG
// section to its left. Sections before the first .ORG are an exception, and are grouped with the nearest .ORG to the
// right. When no .ORG is present, all sections are treated as a single group starting at initial_base_address.
//
// For section after a .ORG, the new <base_address> for each section in a section group is the <group_base_address>
// plus the cumulative size of the processed sections in that group after the .ORG, respecting aligment requirements.
// For sections before a .ORG, the new <base_address> is the .ORG minus the cumulative size, iterating right-to-left
// starting in the .ORG section.
//
// When a .BURN <num> is present, grouping occurs as-if an extra section was append to prog which contains a .ORG <num>.
template <typename Address>
IRMemoryAddressTable<Address> assign_addresses(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                               LineToSymbolType line_to_Symbol,
                                               typename Address::Type initial_base_address = 0) {
  enum class Direction { Forward, Backward } direction = Direction::Forward;
  using addr_t = typename Address::Type;
  static const auto max_address = std::numeric_limits<addr_t>::max();
  static const u64 MODULUS = static_cast<u64>(max_address) + 1;

  // Pre-allocate vector according to the total size of the IR lines in all sections.
  // This overrserves storage---not all IR lines generate object code---but is a stable upper bound and avoid
  // reallocation in the address-assignment loop.
  i64 size = 0;
  for (const auto &[desc, ir] : prog) size += ir.size();
  // ITEMS ARE NOT INSERTED IN SORTED ORDER. DO NOT USE AS A MAP UNTIL SORTING.
  // Since all IR lines across all PepIRProgram have unique (C++) addresses, we can blindly append and sort later.
  // This gives O(1) insert rather than  n* O(nlgn) with the requirement for a manual sort before returning.
  IRMemoryAddressTable<Address> ret;
  ret.container.reserve(size);

  // Process all sections affected by one .ORG before processing any sections affected by the next one.
  // There may be some IR lines in an .ORG section that are before the .ORG and iterating left-to-right lets us assign
  // addresses to those items correctly.
  std::vector<detail::SectionAddrInfo> sorted_work;
  sorted_work.reserve(prog.size());

  // Phase 1: Determine the order in which sections should be assigned addresses.
  // This is generally left-to-right, unless there are sections before the first .ORG or the program contains a .BURN,
  // in which case some subset of sections is assigned right-to-left.

  // Sections before the first .ORG need to be grouped right rather than left
  i64 first_org_section = -1;
  // Record all sections which contain at least one .ORG
  for (int it = 0; it < prog.size(); it++) {
    if (prog[it].first.org_count > 0) {
      sorted_work.emplace_back(detail::SectionAddrInfo{it, it});
      // If this is the first .ORG, we need to insert the the first 0..it sections
      if (first_org_section == -1) {
        first_org_section = it;
        for (int jt = it - 1; jt >= 0; jt--) sorted_work.emplace_back(detail::SectionAddrInfo{jt, jt + 1});
      }
    } else if (first_org_section != -1) sorted_work.emplace_back(detail::SectionAddrInfo{it, it - 1});
  }

  // Program contained no .ORGs. Act as if first section begins with a .ORG <initial_base_address>.
  if (first_org_section == -1) {
    sorted_work.insert(sorted_work.begin(), detail::SectionAddrInfo{0, 0});
    for (int it = 1; it < prog.size(); it++) sorted_work.emplace_back(detail::SectionAddrInfo{it, it - 1});
  }

  if (prog.size() != sorted_work.size()) throw std::logic_error("Layout of sections failed");

  // Phase 2: assign addresses for each section to each line of IR.
  addr_t base_address = 0;

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
          symbol->value = std::make_shared<pepp::core::symbol::AliasValue>(sizeof(addr_t), other);
        } else {
          auto masked_bits = bits::MaskedBits{.byteCount = sizeof(addr_t), .bitPattern = 0, .mask = MODULUS - 1};
          (void)argument->serialize(
              bits::span<u8>{reinterpret_cast<u8 *>(&masked_bits.bitPattern), masked_bits.byteCount},
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
        next_base = (base_address + size) % MODULUS;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        ret.container.emplace_back(line.get(), Address(base_address, size));
        base_address = next_base;
      } else {
        next_base = (base_address - size) % MODULUS;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        auto adjustedAddress = next_base + (size > 0 ? 1 : 0);
        // If we use newBase, we are off-by-one when size is non-zero.
        symbol_base = adjustedAddress;
        ret.container.emplace_back(line.get(), Address(adjustedAddress % MODULUS, size));
        base_address = next_base;
      }
      sec_desc.byte_count += size;

      if (auto line_symbol = line->template typed_attribute<SymbolDeclaration>(); line_symbol) {
        const pepp::core::symbol::Type type = line_to_Symbol(line.get());
        line_symbol->entry->value =
            std::make_shared<pepp::core::symbol::LocationValue>(size, sizeof(addr_t), symbol_base, 0, type);
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
  std::sort(ret.container.begin(), ret.container.end(), detail::IRComparator<Address>{});
  return ret;
}

// Create a lookup data structure that converts IR pointers back to the generated object code.
// Since IR no longer know their own address, we need to cache the object code because it cannot easily be regenerated.
using IR2ObjectPair = std::pair<const LinearIR *, std::span<u8>>;
struct IR2ObjectComparator {
  bool operator()(const IR2ObjectPair &lhs, const IR2ObjectPair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(LinearIR *const lhs, LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ObjectCodeMap = fc::flat_map<std::vector<IR2ObjectPair>, IR2ObjectComparator>;

struct StaticRelocation {
  // Offset into a section's object code (in bytes) which needs relocation.
  // Per ELF spec, needs to be offset for relocatable object files rather than an address to simplify linker.
  u32 section_offset;
  u32 section_idx; // section index in prog, not ELF.
};

struct ProgramObjectCodeResult {
  IR2ObjectCodeMap ir_to_object_code;
  // Group relocations by symbol rather than by section so that we can write the symbol table and relocations
  // simultaneously.
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> relocations;
  // A common arena for all section's object code
  std::vector<u8> object_code;
  struct SectionSpans {
    std::span<u8> object_code;
  };
  // Use section indicies from original "prog" and provides only the object code for a particular section descriptor.
  std::vector<SectionSpans> section_spans;
};

template <typename Address, typename Visitor>
ProgramObjectCodeResult to_object_code(const IRMemoryAddressTable<Address> &addresses,
                                       std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  ProgramObjectCodeResult ret;
  std::vector<detail::SectionOffsets> offsets(prog.size(), detail::SectionOffsets{});
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
    Visitor visitor(addresses, sec.low_address, it, oc_subspan, ret.relocations, ret.ir_to_object_code);
    offset.reloc_offset = ret.relocations.size();
    for (const auto &line : ir) visitor.accept(line.get());
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
} // namespace pepp::tc
