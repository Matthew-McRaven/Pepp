#include "core/formats/elf/managed_to_packed.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
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

// Copy a section's bytes into the storage the packed file allocated for it.
struct CopyContent {
  AStorage &into;
  void operator()(std::monostate) const {}
  void operator()(const NoBits &) const {}
  void operator()(const RawBytes &raw) const { into.append(bits::span<const u8>{raw.bytes.data(), raw.bytes.size()}); }
  void operator()(const ManagedStringTable &table) const {
    std::vector<u8> bytes(table.serialized_size());
    table.serialize(bits::span<u8>{bytes.data(), bytes.size()});
    into.append(bits::span<const u8>{bytes.data(), bytes.size()});
  }
};
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

template <ElfBits B, ElfEndian E> uxword entry_size(const ManagedSection &sec) {
  switch (sec.type) {
  case SectionTypes::SHT_SYMTAB: [[fallthrough]];
  case SectionTypes::SHT_DYNSYM: return sizeof(PackedElfSymbol<B, E>);
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
  const auto *names = shstrtab_section ? std::get_if<ManagedStringTable>(&shstrtab_section->content) : nullptr;
  const auto offset_of_section_name = [&](std::string_view name) -> u32 {
    if (!names) return 0;
    else if (const auto h = names->find(name); !h)
      throw std::logic_error("pack: .shstrtab is missing a section's name");
    else return names->offset_of(*h);
  };

  for (auto section_ref : ordered) {
    const auto *sec = elf.section(section_ref);
    Shdr shdr;
    shdr.sh_name = offset_of_section_name(sec->name);
    shdr.sh_type = bits::to_underlying(sec->type);
    shdr.sh_flags = narrow<B>(bits::to_underlying(sec->flags), "sh_flags");
    shdr.sh_addr = narrow<B>(sec->addr, "sh_addr");
    shdr.sh_size = narrow<B>(sec->sh_size(), "sh_size");
    shdr.sh_addralign = sec->addralign;
    shdr.sh_entsize = narrow<B>(entry_size<B, E>(*sec), "sh_entsize");
    shdr.sh_link = index_of.at(sec->link);
    shdr.sh_info = std::visit(ShInfo{index_of}, sec->info);

    if (const auto index = out.add_section(std::move(shdr)); index != index_of.at(section_ref))
      throw std::logic_error("pack: section index drifted from its order");
    else std::visit(CopyContent{*out.section_data[index]}, sec->content);
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
