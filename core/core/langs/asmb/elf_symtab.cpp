/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "elf_symtab.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/packed_access_relocations.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "spdlog/spdlog.h"

namespace {
using namespace pepp::bts;

// An undefined symbol must be defined by another object file, so it must be visible to them.
SymbolBinding binding_of(const pepp::core::symbol::Entry &entry) {
  switch (entry.binding) {
  case pepp::core::symbol::Binding::Global: return SymbolBinding::STB_GLOBAL;
  case pepp::core::symbol::Binding::Weak: return SymbolBinding::STB_WEAK;
  default: return entry.is_undefined() ? SymbolBinding::STB_GLOBAL : SymbolBinding::STB_LOCAL;
  }
}

SymbolType type_of(const pepp::core::symbol::Entry &entry) {
  if (!entry.value) return SymbolType::STT_NOTYPE;
  switch (entry.value->type()) {
  case pepp::core::symbol::Type::Code: return SymbolType::STT_FUNC;
  case pepp::core::symbol::Type::Object: [[fallthrough]];
  case pepp::core::symbol::Type::Constant: return SymbolType::STT_OBJECT;
  case pepp::core::symbol::Type::Section: return SymbolType::STT_SECTION;
  default: return SymbolType::STT_NOTYPE;
  }
}

u64 value_of(const pepp::core::symbol::Entry &entry) { return entry.value ? entry.value->value()() : 0; }
u64 size_of(const pepp::core::symbol::Entry &entry) { return entry.value ? entry.value->size() : 0; }

// A constant belongs to no section, and an undefined symbol has none to point at.
u16 shndx_of(const pepp::core::symbol::Entry &entry) {
  if (entry.is_undefined()) return bits::to_underlying(SectionIndices::SHN_UNDEF);
  else if (entry.is_constant()) return bits::to_underlying(SectionIndices::SHN_ABS);
  return entry.section_index;
}

SymbolVisibility visibility_of(pepp::core::symbol::Visibility visibility) {
  switch (visibility) {
  case pepp::core::symbol::Visibility::Internal: return SymbolVisibility::STV_INTERNAL;
  case pepp::core::symbol::Visibility::Hidden: return SymbolVisibility::STV_HIDDEN;
  case pepp::core::symbol::Visibility::Protected: return SymbolVisibility::STV_PROTECTED;
  default: return SymbolVisibility::STV_DEFAULT;
  }
}

struct WrittenSymbols {
  u16 symtab;
  std::map<const pepp::core::symbol::Entry *, u32> index;
  std::vector<u32> section; // Section symbol index in symtab for each section header index or 0 if non-existent.
};

using Program = std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>>;

// The address assigned to the section by the assembler. Nullopt for constant & undefined symbols.
std::optional<u64> section_low_address(const pepp::core::symbol::Entry &entry, const Program &prog) {
  const u16 shndx = shndx_of(entry), base = pepp::tc::SectionDescriptor::section_base_index;
  if (shndx < base || shndx - base >= prog.size()) return std::nullopt;
  return prog[shndx - base].first.low_address;
}

// Symbols that must not be replaced with section-relative relocation entries.
std::set<const pepp::core::symbol::Entry *> preserved_symbols(const pepp::tc::ProgramObjectCodeResult &object_code) {
  std::set<const pepp::core::symbol::Entry *> ret;
  for (const auto &relocations : object_code.relocations)
    for (const auto &relocation : relocations)
      if (relocation.preserve_symbol) ret.insert(relocation.target.symbol.get());
  return ret;
}

// Must be written after all sections which define symbols to avoid having to update st_shndx values later.
template <ElfBits B, ElfEndian E>
WrittenSymbols write_symbols(PackedGrowableElfFile<B, E> &elf, const pepp::core::symbol::LeafTable &table,
                             const std::set<const pepp::core::symbol::Entry *> &preserved, const Program &prog) {
  using namespace pepp::core::symbol;
  using Symbol = PackedElfSymbol<B, E>;
  std::vector<LeafTable::entry_ptr_t> symbols;
  enumerate(table, symbols);
  // Erase local labels that are not referenced by relocations.
  std::erase_if(symbols, [&](const auto &entry) {
    if (entry->value && entry->value->type() == Type::Deleted) return true;
    const bool local_label = binding_of(*entry) == SymbolBinding::STB_LOCAL && entry->name.starts_with(".L");
    return local_label && !preserved.contains(entry.get());
  });
  // ELF requires every local precede any non-local. Within partitions, sort by name to make output deterministic.
  std::sort(symbols.begin(), symbols.end(), [](const auto &lhs, const auto &rhs) {
    const bool left_local = binding_of(*lhs) == SymbolBinding::STB_LOCAL;
    const bool right_local = binding_of(*rhs) == SymbolBinding::STB_LOCAL;
    if (left_local != right_local) return left_local;
    return lhs->name < rhs->name;
  });

  const auto strtab = add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB);
  const auto symtab = add_named_symtab(elf, ".symtab", strtab);
  elf.section_headers[symtab].sh_addralign = sizeof(word<B>);
  PackedSymbolWriter<B, E> writer(elf, symtab);
  WrittenSymbols ret{.symtab = symtab, .index = {}, .section = std::vector<u32>(elf.section_headers.size(), 0)};
  // Create a STT_SECTION symbol for each allocatable section, and for the reserved DWARF sections so that their
  // relocations have a target.
  for (u16 index = 1; index < elf.section_headers.size(); ++index) {
    const word<B> flags = elf.section_headers[index].sh_flags;
    const bool progbits = elf.section_headers[index].sh_type == bits::to_underlying(SectionTypes::SHT_PROGBITS);
    if ((flags & bits::to_underlying(SectionFlags::SHF_ALLOC)) == 0 && !progbits) continue;
    Symbol symbol;
    // Assign the base address of the section as the section symbol's value. This makes it trivial to compute Pep/10
    // S+A relocations.
    symbol.st_value = elf.section_headers[index].sh_addr;
    symbol.st_shndx = index;
    symbol.set_type(SymbolType::STT_SECTION);
    symbol.set_bind(SymbolBinding::STB_LOCAL);
    ret.section[index] = writer.add_symbol(std::move(symbol));
  }
  for (const auto &ptr : symbols) {
    const auto &entry = *ptr;
    Symbol symbol;
    u64 value = value_of(entry);
    // Convert the symbol into a section-relative offset.
    if (const auto low = section_low_address(entry, prog); low)
      value = static_cast<u64>(elf.section_headers[shndx_of(entry)].sh_addr) + (value - *low);
    symbol.st_value = static_cast<word<B>>(value);
    symbol.st_size = static_cast<word<B>>(size_of(entry));
    symbol.st_shndx = shndx_of(entry);
    symbol.set_type(type_of(entry));
    symbol.set_bind(binding_of(entry));
    symbol.set_visibility(visibility_of(entry.visibility));
    ret.index[&entry] = writer.add_symbol(std::move(symbol), entry.name);
  }
  // Already sorted (no indices move) but this will update sh_info to point to the first non-local symbol.
  if (writer.symbol_count() > 0) writer.arrange_local_symbols();
  return ret;
}

// Must follow write_symbols, since relocations name their symbol by index.
template <ElfBits B, ElfEndian E>
void write_relocations(PackedGrowableElfFile<B, E> &elf, const Program &prog,
                       const pepp::tc::ProgramObjectCodeResult &object_code, const WrittenSymbols &symbols) {
  for (u32 it = 0; it < prog.size(); it++) {
    const auto &relocs = object_code.relocations[it];
    if (relocs.empty()) continue;
    const auto &desc = prog[it].first;
    const auto rela = add_named_rela(elf, ".rela" + desc.name, symbols.symtab, desc.section_index);
    elf.section_headers[rela].sh_flags = bits::to_underlying(SectionFlags::SHF_INFO_LINK);
    elf.section_headers[rela].sh_addralign = sizeof(word<B>);
    PackedRelocationWriter<B, E> writer(elf, rela);
    for (const auto &rel : relocs) {
      const auto &entry = *rel.target.symbol;
      // Local symbols (other than section symbols) might be dropped. Give them an addend which is equal to the distance
      // between the section symbol and the local symbol. In the local case, replace the requested symbol with the
      // section symbol unless the assembler specifically requested the symbol be preserved as-is.
      const bool local = !rel.preserve_symbol && binding_of(entry) == SymbolBinding::STB_LOCAL;
      const u32 symbol = local ? symbols.section.at(entry.section_index) : symbols.index.at(&entry);
      if (symbol == 0) throw std::logic_error("No symbol to relocate against");
      sword<B> addend = 0;
      if (local) {
        if (const auto low = section_low_address(entry, prog); !low)
          throw std::logic_error("Local relocation target has no section");
        else addend = static_cast<sword<B>>(value_of(entry) - *low);
      }
      addend += static_cast<sword<B>>(rel.target.addend); // From `symbol + constant`.
      writer.add_rela(rel.section_offset, rel.type, symbol, addend);
    }
  }
}

template <ElfBits B, ElfEndian E>
pepp::tc::ElfResult build(ElfFileType type, ElfMachineType machine, const Program &prog,
                          const pepp::tc::ProgramObjectCodeResult &object_code,
                          const pepp::core::symbol::LeafTable &symbols) {
  pepp::tc::ElfResult ret;
  using File = PackedGrowableElfFile<B, E>;
  using enum ElfFileType;
  using enum ElfABI;
  // emplace returns the alternative it built, so the variant owns the file and we keep a typed pointer to fill it.
  auto *elf = ret.elf.emplace<std::unique_ptr<File>>(std::make_unique<File>(type, machine, ELFOSABI_NONE)).get();
  // The null section and .shstrtab come first; SectionDescriptor::section_base_index accounts for them.
  ensure_section_header_table(*elf);

  // An ET_REL's sections start at 0
  const bool relocatable = type == ET_REL;

  for (u32 it = 0; it < prog.size(); it++) {
    const auto &desc = prog[it].first;
    SPDLOG_INFO("{} creating", desc.name);
    const auto sh_type = desc.flags.z ? SectionTypes::SHT_NOBITS : SectionTypes::SHT_PROGBITS;
    const auto index = add_named_section(*elf, desc.name, sh_type);
    if (index != desc.section_index) throw std::logic_error("Mismatch in pre-computed section index");

    auto &shdr = elf->section_headers[index];
    // Every section from the AST is bits in memory, so all of them are allocated.
    auto flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    if (desc.flags.x) flags |= bits::to_underlying(SectionFlags::SHF_EXECINSTR);
    if (desc.flags.w) flags |= bits::to_underlying(SectionFlags::SHF_WRITE);
    shdr.sh_flags = flags;
    shdr.sh_addr = relocatable ? 0 : desc.low_address;
    shdr.sh_addralign = desc.alignment;
    // Layout sizes sections from their data, which NOBITS has none of.
    if (desc.flags.z) shdr.sh_size = desc.high_address - desc.low_address;
    // Shares the assembler's object code rather than copying it.
    else elf->section_data[index] = std::make_shared<BlockStorage::BlockStorageSlice>(object_code.section_slices[it]);
  }
  // Program headers are not needed for ET_REL files.
  if (!relocatable) {
    const auto from = pepp::tc::SectionDescriptor::section_base_index;
    for (const auto &[flags, constraint] : group_load_segments(*elf, from, static_cast<u16>(from + prog.size()))) {
      elf->add_segment(SegmentType::PT_LOAD, flags);
      ret.segments.push_back(constraint);
    }
  }

  // write_debug_line fills these later, but their section symbols must exist before .symtab is written.
  for (const auto *name : pepp::tc::debugSections) {
    const auto index = add_named_section(*elf, name, SectionTypes::SHT_PROGBITS);
    elf->section_headers[index].sh_addralign = 1;
  }
  const auto written = write_symbols(*elf, symbols, preserved_symbols(object_code), prog);
  write_relocations(*elf, prog, object_code, written);
  return ret;
}
} // namespace

pepp::tc::ElfResult pepp::tc::sections_to_elf(ElfBits bits, ElfEndian endian, ElfMachineType machine,
                                              const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                              const ProgramObjectCodeResult &object_code,
                                              const pepp::core::symbol::LeafTable &symbols, ElfFileType type) {
  using enum ElfBits;
  using enum ElfEndian;
  if (type != ElfFileType::ET_EXEC && type != ElfFileType::ET_REL)
    throw std::logic_error("sections_to_elf: only supports ET_EXEC and ET_REL");
  else if (bits == b32 && endian == le) return build<b32, le>(type, machine, prog, object_code, symbols);
  else if (bits == b32) return build<b32, be>(type, machine, prog, object_code, symbols);
  else if (endian == le) return build<b64, le>(type, machine, prog, object_code, symbols);
  else return build<b64, be>(type, machine, prog, object_code, symbols);
}

std::vector<u8> pepp::tc::elf_bytes(ElfResult &result) { return pepp::bts::elf_bytes(result.elf, &result.segments); }

void pepp::tc::write_elf(ElfResult &result, std::ostream &out) {
  pepp::bts::write_elf(result.elf, out, &result.segments);
}

pepp::bts::AnyInputElf pepp::tc::to_input_elf(ElfResult &result, std::optional<std::string> path) {
  return pepp::bts::to_input_elf(result.elf, &result.segments, std::move(path));
}
