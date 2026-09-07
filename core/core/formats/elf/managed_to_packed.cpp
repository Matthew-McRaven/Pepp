#include "core/formats/elf/managed_to_packed.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"
#include "core/formats/elf/managed_section_symtab.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_storage.hpp"

namespace {
using namespace pepp::bts;

// Sections are written in ascending address order, with the exception of those with no address. Those are sorted to be
// last. This should have the effect of placing all sections needed by the loader at the front of the file, and
// linker-only information towards the end.
std::vector<SectionRef> sorted_write_order(const ManagedElf &elf, std::span<const SectionRef> live) {
  std::vector<SectionRef> ret(live.begin(), live.end());
  auto comp = [&](SectionRef lhs, SectionRef rhs) {
    const auto left = elf.section(lhs)->addr, right = elf.section(rhs)->addr;
    if ((left == 0) != (right == 0)) return right == 0;
    return left < right;
  };
  std::stable_sort(ret.begin(), ret.end(), comp);
  return ret;
}

// Extract the integer value for sh_info, which includes converting from a SectionRef to the assigned index.
struct ShInfo {
  const std::map<SectionRef, u16> &index_of;
  u32 operator()(u32 count) const { return count; }
  u32 operator()(SectionRef ref) const { return index_of.at(ref); }
};

// Narrow to the target word size, refusing to truncate.
template <ElfBits B> word<B> narrow(uxword value, const char *what) {
  if constexpr (B == ElfBits::b32) {
    if (value > 0xFFFFFFFFull) throw std::logic_error(std::string("pack: ") + what + " does not fit a 32-bit field");
  }
  return static_cast<word<B>>(value);
}

SymbolBinding binding_of(pepp::core::symbol::Binding binding) {
  switch (binding) {
  case pepp::core::symbol::Binding::Global: return SymbolBinding::STB_GLOBAL;
  case pepp::core::symbol::Binding::Weak: return SymbolBinding::STB_WEAK;
  default: return SymbolBinding::STB_LOCAL;
  }
}

using SymbolEntry = ManagedSymbolTable::entry_ptr_t;

bool is_constant(const SymbolEntry &entry) {
  return entry->value && entry->value->type() == pepp::core::symbol::Type::Constant;
}

SymbolType type_of(const SymbolEntry &entry) {
  if (!entry->value) return SymbolType::STT_NOTYPE;
  switch (entry->value->type()) {
  case pepp::core::symbol::Type::Code: return SymbolType::STT_FUNC;
  case pepp::core::symbol::Type::Object: [[fallthrough]];
  case pepp::core::symbol::Type::Constant: return SymbolType::STT_OBJECT;
  default: return SymbolType::STT_NOTYPE;
  }
}

uxword value_of(const SymbolEntry &entry) { return entry->value ? entry->value->value()() : 0; }
uxword size_of(const SymbolEntry &entry) { return entry->value ? entry->value->size() : 0; }

SymbolVisibility visibility_of(pepp::core::symbol::Visibility visibility) {
  switch (visibility) {
  case pepp::core::symbol::Visibility::Internal: return SymbolVisibility::STV_INTERNAL;
  case pepp::core::symbol::Visibility::Hidden: return SymbolVisibility::STV_HIDDEN;
  case pepp::core::symbol::Visibility::Protected: return SymbolVisibility::STV_PROTECTED;
  default: return SymbolVisibility::STV_DEFAULT;
  }
}

// Copy a section's bytes into the storage the packed file allocated for it.
template <ElfBits B, ElfEndian E> struct CopyContent {
  AStorage &into;
  const ManagedElf &elf;
  const ManagedSection &sec;
  const std::map<SectionRef, u16> &index_of;

  void operator()(std::monostate) const {}
  void operator()(const NoBits &) const {}
  void operator()(const RawBytes &raw) const { into.append(bits::span<const u8>{raw.bytes.data(), raw.bytes.size()}); }
  // One boxed alternative means the compiler no longer checks every payload kind is handled, so the
  // chain ends in a throw rather than falling through and writing nothing.
  void operator()(const std::unique_ptr<ManagedPayload> &payload) const {
    if (!payload) return;
    else if (const auto *table = dynamic_cast<const ManagedStringTable *>(payload.get())) write_strings(*table);
    else if (const auto *table = dynamic_cast<const ManagedSymbolTable *>(payload.get())) write_symbols(*table);
    else throw std::logic_error("pack: a section holds a payload this does not know how to write");
  }

  void write_strings(const ManagedStringTable &table) const {
    std::vector<u8> bytes(table.serialized_size());
    table.serialize(bits::span<u8>{bytes.data(), bytes.size()});
    into.append(bits::span<const u8>{bytes.data(), bytes.size()});
  }

  void write_symbols(const ManagedSymbolTable &table) const {
    const auto *maybe_strtab = elf.section(sec.link);
    const auto *names = maybe_strtab ? maybe_strtab->template content_as<ManagedStringTable>() : nullptr;
    const auto name_of = [&](std::string_view name) -> u32 {
      if (!names) return 0;
      else if (const auto h = names->find(name); !h) throw std::logic_error("pack: symbol name not in strtab");
      else return names->offset_of(*h);
    };
    const auto section_of = [&](const SymbolEntry &entry) -> u16 {
      if (is_constant(entry)) return static_cast<u16>(SectionIndices::SHN_ABS);
      else return index_of.at(table.section_of(entry));
    };

    for (const auto &entry : table.frozen_order()) {
      PackedElfSymbol<B, E> symbol; // Initialized to all zeros, as per Figure 1-18.
      if (!entry) { // nullptr will receive a null symbol, which covers the index 0 case.
        into.append(symbol);
        continue;
      }
      symbol.st_name = name_of(entry->name);
      symbol.st_value = narrow<B>(value_of(entry), "st_value");
      symbol.st_size = narrow<B>(size_of(entry), "st_size");
      symbol.st_shndx = section_of(entry);
      symbol.set_type(type_of(entry));
      symbol.set_bind(binding_of(entry->binding));
      symbol.set_visibility(visibility_of(entry->visibility));
      into.append(symbol);
    }
  }
};

template <ElfBits B, ElfEndian E> uxword entry_size(const ManagedSection &sec) {
  switch (sec.type) {
  case SectionTypes::SHT_SYMTAB: [[fallthrough]];
  case SectionTypes::SHT_DYNSYM: return symbol_bytes(B);
  case SectionTypes::SHT_REL: return sizeof(PackedElfRel<B, E>);
  case SectionTypes::SHT_RELA: return sizeof(PackedElfRelA<B, E>);
  default: return 0;
  }
}

template <ElfBits B, ElfEndian E>
std::map<SectionRef, u16> pack_into(const ManagedElf &elf, std::span<const SectionRef> live,
                                     PackedGrowableElfFile<B, E> &out) {
  using Ehdr = typename PackedElf<B, E>::Ehdr;
  using Shdr = typename PackedElf<B, E>::Shdr;

  // Replaces the whole header, so it must precede add_section(), which maintains e_shnum and e_shentsize.
  // e_shentsize is only assigned for the first section, so a later overwrite would leave it 0 for good.
  out.header = Ehdr(elf.type, elf.machine, elf.abi, elf.abi_version);
  out.header.e_flags = elf.flags;
  out.header.e_entry = narrow<B>(elf.entry, "e_entry");

  // Pseudo-sections above SHN_HIOS are not real sections, but instead magic sentinel values.
  // Pre-insert them so that they are usable as sh_link/info targets.
  std::map<SectionRef, u16> index_of{
      {ManagedElf::SHN_UNDEF, static_cast<u16>(SectionIndices::SHN_UNDEF)},
      {ManagedElf::SHN_ABS, static_cast<u16>(SectionIndices::SHN_ABS)},
      {ManagedElf::SHN_COMMON, static_cast<u16>(SectionIndices::SHN_COMMON)},
  };
  // SHN_UNDEF must point to a null header. Requires out.header to be set first.
  out.add_section(create_null_header<B, E>());
  const auto ordered = sorted_write_order(elf, live);
  // Offset by 1 to account for existence of null header.
  for (u32 it = 0; it < ordered.size(); it++) index_of[ordered[it]] = static_cast<u16>(it + 1);
  out.header.e_shstrndx = index_of.at(elf.shstrtab());

  // While creating our section headers, we need to resolve the section names into offsets in our section header string
  // table. shstrtab is optional, in which case e_shstrndx is SHN_UNDEF and all sh_name fields are 0.
  const auto *shstrtab_section = elf.section(elf.shstrtab());
  const auto *names = shstrtab_section ? shstrtab_section->content_as<ManagedStringTable>() : nullptr;
  const auto offset_of_section_name = [&](std::string_view name) -> u32 {
    if (!names) return 0;
    else if (const auto h = names->find(name); !h) throw std::logic_error("pack: .shstrtab missing section's name");
    else return names->offset_of(*h);
  };

  for (auto section_ref : ordered) {
    const auto *sec = elf.section(section_ref);
    Shdr shdr;
    shdr.sh_name = offset_of_section_name(sec->name);
    shdr.sh_type = bits::to_underlying(sec->type);
    shdr.sh_flags = narrow<B>(bits::to_underlying(sec->flags), "sh_flags");
    shdr.sh_addr = narrow<B>(sec->addr, "sh_addr");
    shdr.sh_size = narrow<B>(sec->sh_size(B), "sh_size");
    shdr.sh_addralign = sec->addralign;
    shdr.sh_entsize = narrow<B>(entry_size<B, E>(*sec), "sh_entsize");
    shdr.sh_link = index_of.at(sec->link);
    shdr.sh_info = std::visit(ShInfo{index_of}, sec->info);

    if (const auto index = out.add_section(std::move(shdr)); index != index_of.at(section_ref))
      throw std::logic_error("pack: section index drifted from its order");
    else std::visit(CopyContent<B, E>{*out.section_data[index], elf, *sec, index_of}, sec->content);
  }

  return index_of;
}

template <ElfBits B, ElfEndian E> PackResult pack_as(const ManagedElf &elf, std::span<const SectionRef> live) {
  auto packed = std::make_unique<PackedGrowableElfFile<B, E>>(elf.type, elf.machine, elf.abi);
  auto indices = pack_into(elf, live, *packed);
  return PackResult{std::move(packed), std::move(indices)};
}
} // namespace

pepp::bts::PackResult pepp::bts::pack(const ManagedElf &elf, std::span<const SectionRef> live) {
  using enum ElfBits;
  using enum ElfEndian;
  // Use helpers to avoid the caller having to pick the right template instantiation.
  switch (elf.bits()) {
  case b32: return elf.endian() == le ? pack_as<b32, le>(elf, live) : pack_as<b32, be>(elf, live);
  case b64: return elf.endian() == le ? pack_as<b64, le>(elf, live) : pack_as<b64, be>(elf, live);
  }
  throw std::logic_error("pack: unknown word size");
}
