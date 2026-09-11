#include "elf_symtab.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/packed_access_relocations.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "spdlog/spdlog.h"

namespace {
using namespace pepp::bts;

// The segment being grown, if any. A constraint names an index range with ascending addresses, so a section may only
// join when it starts exactly where the segment ends; that also keeps address gaps out of the file.
struct Run {
  u32 flags, end;
  bool has_nobits;
};

// An undefined symbol must be defined by another object file, so it must be visible to them.
SymbolBinding binding_of(const pepp::core::symbol::Entry &entry) {
  switch (entry.binding) {
  case pepp::core::symbol::Binding::Global: return SymbolBinding::STB_GLOBAL;
  case pepp::core::symbol::Binding::Weak: return SymbolBinding::STB_WEAK;
  default: return entry.is_undefined() ? SymbolBinding::STB_GLOBAL : SymbolBinding::STB_LOCAL;
  }
}

bool is_constant(const pepp::core::symbol::Entry &entry) {
  return entry.value && entry.value->type() == pepp::core::symbol::Type::Constant;
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
  else if (is_constant(entry)) return bits::to_underlying(SectionIndices::SHN_ABS);
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

// Must be written after all sections which define symbols to avoid having to update st_shndx values later.
template <ElfBits B, ElfEndian E>
WrittenSymbols write_symbols(PackedGrowableElfFile<B, E> &elf, const pepp::core::symbol::LeafTable &table) {
  using namespace pepp::core::symbol;
  using Symbol = PackedElfSymbol<B, E>;
  std::vector<LeafTable::entry_ptr_t> symbols;
  enumerate(table, symbols);
  std::erase_if(symbols, [](const auto &entry) { return entry->value && entry->value->type() == Type::Deleted; });
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
  // Create a STT_SECTION symbol for each allocatable section
  for (u16 index = 1; index < elf.section_headers.size(); ++index) {
    const word<B> flags = elf.section_headers[index].sh_flags;
    if ((flags & bits::to_underlying(SectionFlags::SHF_ALLOC)) == 0) continue;
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
    symbol.st_value = static_cast<word<B>>(value_of(entry));
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
void write_relocations(PackedGrowableElfFile<B, E> &elf,
                       const std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>> &prog,
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
      const auto &entry = *rel.symbol;
      // Local symbols (other than section symbols) might be dropped. Give them an addend which is equal to the distance
      // between the section symbol and the local symbol. In the local case, replace the requested symbol with the
      // section symbol.
      const bool local = binding_of(entry) == SymbolBinding::STB_LOCAL;
      const u32 symbol = local ? symbols.section.at(entry.section_index) : symbols.index.at(&entry);
      if (symbol == 0) throw std::logic_error("No symbol to relocate against");
      sword<B> addend = 0;
      if (local) {
        const word<B> base = elf.section_headers[entry.section_index].sh_addr;
        addend = static_cast<sword<B>>(value_of(entry) - base);
      }
      writer.add_rela(rel.section_offset, rel.type, symbol, addend);
    }
  }
}

template <ElfBits B, ElfEndian E>
pepp::tc::ElfResult build(ElfMachineType machine,
                          const std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>> &prog,
                          const pepp::tc::ProgramObjectCodeResult &object_code,
                          const pepp::core::symbol::LeafTable &symbols) {
  pepp::tc::ElfResult ret;
  using File = PackedGrowableElfFile<B, E>;
  using enum ElfFileType;
  using enum ElfABI;
  // emplace returns the alternative it built, so the variant owns the file and we keep a typed pointer to fill it.
  auto *elf = ret.elf.emplace<std::unique_ptr<File>>(std::make_unique<File>(ET_EXEC, machine, ELFOSABI_NONE)).get();
  // The null section and .shstrtab come first; SectionDescriptor::section_base_index accounts for them.
  ensure_section_header_table(*elf);

  std::optional<Run> run;

  for (u32 it = 0; it < prog.size(); it++) {
    const auto &desc = prog[it].first;
    SPDLOG_INFO("{} creating", desc.name);
    const auto type = desc.flags.z ? SectionTypes::SHT_NOBITS : SectionTypes::SHT_PROGBITS;
    const auto index = add_named_section(*elf, desc.name, type);
    if (index != desc.section_index) throw std::logic_error("Mismatch in pre-computed section index");

    auto &shdr = elf->section_headers[index];
    // Every section from the AST is bits in memory, so all of them are allocated.
    auto flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    if (desc.flags.x) flags |= bits::to_underlying(SectionFlags::SHF_EXECINSTR);
    if (desc.flags.w) flags |= bits::to_underlying(SectionFlags::SHF_WRITE);
    shdr.sh_flags = flags;
    shdr.sh_addr = desc.low_address;
    shdr.sh_addralign = desc.alignment;
    // Layout sizes sections from their data, which NOBITS has none of.
    // Extent as every reader sees it. high_address - low_address can overstate a section once .ORG is involved.
    u32 extent = 0;
    if (desc.flags.z) {
      extent = desc.high_address - desc.low_address;
      shdr.sh_size = extent;
    } else {
      const auto bytes = object_code.section_spans[it].object_code;
      elf->section_data[index]->append(bits::span<const u8>{bytes.data(), bytes.size()});
      extent = static_cast<u32>(bytes.size());
    }
    const u32 end = desc.low_address + extent;

    u32 load_flags = 0;
    if (desc.flags.r) load_flags |= bits::to_underlying(SegmentFlags::PF_R);
    if (desc.flags.w) load_flags |= bits::to_underlying(SegmentFlags::PF_W);
    if (desc.flags.x) load_flags |= bits::to_underlying(SegmentFlags::PF_X);
    const u16 align = std::max<u16>(desc.alignment, 1);
    const bool occupies_memory = extent > 0;
    // File data after a NOBITS would sit past p_filesz. A trailing NOBITS is fine.
    const bool blocked = run && run->has_nobits && !desc.flags.z && occupies_memory;
    // We can extend the run if: matching flags, contiguous  addresses, and no intervening NOBITS.
    if (run && run->flags == load_flags && !blocked && desc.low_address == bits::align_up(run->end, align)) {
      auto &segment = ret.segments.back();
      segment.to_sec = index;
      segment.alignment = std::max(segment.alignment, align);
      run->end = end;
      run->has_nobits |= desc.flags.z;
    } else if (occupies_memory) {
      SegmentLayoutConstraint segment;
      segment.alignment = align;
      segment.from_sec = segment.to_sec = index;
      segment.update_sec_addrs = false; // Addresses come from assign_addresses.
      elf->add_segment(SegmentType::PT_LOAD, static_cast<SegmentFlags>(load_flags));
      ret.segments.push_back(segment);
      run = Run{load_flags, end, desc.flags.z};
    } else { // A section with no memory usage cannot be part of a segment.
      run.reset();
    }
  }

  const auto written = write_symbols(*elf, symbols);
  write_relocations(*elf, prog, object_code, written);
  return ret;
}
} // namespace

pepp::tc::ElfResult pepp::tc::sections_to_elf(ElfBits bits, ElfEndian endian, ElfMachineType machine,
                                              const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                              const ProgramObjectCodeResult &object_code,
                                              const pepp::core::symbol::LeafTable &symbols) {
  using enum ElfBits;
  using enum ElfEndian;
  if (bits == b32 && endian == le) return build<b32, le>(machine, prog, object_code, symbols);
  else if (bits == b32) return build<b32, be>(machine, prog, object_code, symbols);
  else if (endian == le) return build<b64, le>(machine, prog, object_code, symbols);
  else return build<b64, be>(machine, prog, object_code, symbols);
}

std::vector<u8> pepp::tc::elf_bytes(ElfResult &result) {
  auto visitor = [&result](auto &file) -> std::vector<u8> {
    if (!file) return {};
    auto layout = pepp::bts::calculate_layout(*file, &result.segments);
    std::vector<u8> ret(pepp::bts::size_for_layout(layout), 0);
    pepp::bts::write(ret, layout);
    return ret;
  };
  return std::visit(visitor, result.elf);
}
