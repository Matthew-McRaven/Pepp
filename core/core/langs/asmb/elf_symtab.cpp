#include "elf_symtab.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/elfio_utils.hpp"
#include "elfio/elf_types.hpp"
#include "elfio/elfio.hpp"

#include <list>

static const std::string rel_name = ".rel";
static ELFIO::section *get_or_create_rel(ELFIO::elfio &elf, const std::string &suffix) {

  // If suffix is empty, just use rel_name. Otherwise, if suffix begins with a full stop, do not insert a full stop.
  // If the suffix does not begin with a full stop, do not insert it.
  auto full_name =
      suffix.empty() ? rel_name : (suffix.starts_with(".") ? rel_name + suffix : (rel_name + ".") + suffix);
  for (auto &sec : elf.sections)
    if (sec->get_name() == full_name && sec->get_type() == ELFIO::SHT_REL) return sec.get();

  ELFIO::section *ret = elf.sections.add(full_name);
  ret->set_type(ELFIO::SHT_REL);
  return ret;
};

static u16 ir_to_elf_section_index(const pepp::tc::ElfResult &elf_wrapper, u16 ir_index) {
  using namespace pepp::tc;
  // Some IR sections are not emitted to ELF because they contained no meaningful data.
  const auto adjustment = elf_wrapper.section_offsets[ir_index];
  return SectionDescriptor::section_base_index + ir_index - adjustment;
}

// Our ELF symbols already bake in pepp::tc::SectionDescriptor::section_base_index, which ir_to_elf_section_index adds
// in again.
static u16 symbol_to_elf_section_index(const pepp::tc::ElfResult &elf_wrapper, const pepp::core::symbol::Entry *entry) {
  using namespace pepp::tc;
  // Our sections inserted into ELF do not start at 0, they start at SectionDescriptor::section_base_index.
  // section_offsets starts at 0, so we need to convert before indexing or risk an out-of-bounds access.
  auto probable_elf_idx = entry->section_index;
  // Entries less than > SHN_ABS and <= SHN_LORESERVE should not be adjusted.
  if (probable_elf_idx < SectionDescriptor::section_base_index || probable_elf_idx >= ELFIO::SHN_LORESERVE)
    return probable_elf_idx;
  // Otherwise we need to convert IR section numbers to actual ELF sections
  return ir_to_elf_section_index(elf_wrapper, probable_elf_idx - SectionDescriptor::section_base_index);
};

void pepp::tc::write_symbol_table(ElfResult &elf_wrapper, pepp::core::symbol::LeafTable &symbol_table,
                                  const ProgramObjectCodeResult &oc, const std::string name) {
  auto &elf = *elf_wrapper.elf.get();
  auto strTab = pepp::tc::addStrTab(elf);
  auto symTab = elf.sections.add(name);
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
  const auto pool = symbol_table.pool();
  for (const auto &[name_idx, entry] : symbol_table.entries()) {
    const auto name = *pool->find(name_idx);
    auto nameIdx = findOrCreateStr(std::string{name});
    // Symbol index of the inserted symbol. Retain to make writing relocations easier.
    ELFIO::Elf_Word symbol_idx = 0;
    // Fast path for undefined symbols
    if (entry->is_undefined()) {
      static constexpr u8 info = (ELFIO::STB_LOCAL << 4) + (ELFIO::STT_NOTYPE & 0xf);
      symbol_idx = symAc.add_symbol(nameIdx, 0, 0, info, 0, ELFIO::SHN_UNDEF);
    } else {
      auto secIdx = symbol_to_elf_section_index(elf_wrapper, entry.get());
      auto value = entry->value;

      u8 type = ELFIO::STT_NOTYPE;
      using Type = pepp::core::symbol::Type;
      if (value->type() == Type::Code) type = ELFIO::STT_FUNC;
      else if (value->type() == Type::Object) type = ELFIO::STT_OBJECT;
      else if (value->type() == Type::Constant) {
        type = ELFIO::STT_OBJECT;
        secIdx = ELFIO::SHN_ABS;
      }

      u8 bind = ELFIO::STB_LOCAL;
      using Binding = pepp::core::symbol::Binding;
      if (entry->binding == Binding::Global) bind = ELFIO::STB_GLOBAL;
      else if (entry->binding == Binding::Weak) bind = ELFIO::STB_WEAK;

      u8 info = (bind << 4) + (type & 0xf);

      u8 vis = ELFIO::STV_DEFAULT;
      switch (entry->visibility) {
      case pepp::core::symbol::Visibility::Default: vis = ELFIO::STV_DEFAULT; break;
      case pepp::core::symbol::Visibility::Hidden: vis = ELFIO::STV_HIDDEN; break;
      case pepp::core::symbol::Visibility::Protected: vis = ELFIO::STV_PROTECTED; break;
      case core::symbol::Visibility::Internal: vis = ELFIO::STV_INTERNAL; break;
      }

      symbol_idx = symAc.add_symbol(nameIdx, value->value()(), entry->value->size(), info, vis, secIdx);
    }
    // For all sections, for all relocations entries against this symbol
    // Create a relocation section for the current section if it does not exist, and append a relocation entry.
    auto relocs_for = oc.relocations.equal_range(entry);
    for (auto rel = relocs_for.first; rel != relocs_for.second; ++rel) {
      const auto ir_idx = rel->second.section_idx;
      const auto elf_idx = ir_to_elf_section_index(elf_wrapper, ir_idx);
      auto relocated_sec = elf_wrapper.elf->sections[elf_idx];
      auto relocation_section = get_or_create_rel(*elf_wrapper.elf, relocated_sec->get_name());
      // Freshly created relocation sections are missing various required fields.
      if (relocation_section->get_info() == 0) relocation_section->set_info(relocated_sec->get_index());
      if (relocation_section->get_link() == 0) relocation_section->set_link(symTab->get_index());
      // BUG: the library should know the size of REL entries when the section is created. However, this field
      // is only initialized on save. Given that swap_symbols depends on entry_size, we need to fill it in NOW.
      // TODO: should not be a magic constant! Should be computed from the size of some struct.
      if (relocation_section->get_entry_size() == 0) relocation_section->set_entry_size(8);
      auto reloc_ac = ELFIO::relocation_section_accessor(*elf_wrapper.elf, relocation_section);
      reloc_ac.add_entry(rel->second.section_offset, symbol_idx, 0);
    }
  }
  // Helper to propogate swapping all symbols in the relocation sections
  // Create a (temporary) accessor for each REL
  std::list<ELFIO::relocation_section_accessor> acs;
  for (auto &sec : elf_wrapper.elf->sections)
    if (sec->get_type() == ELFIO::SHT_REL)
      acs.emplace_back(ELFIO::relocation_section_accessor(*elf_wrapper.elf, sec.get()));

  auto all_swap = [&acs](ELFIO::Elf_Xword first, ELFIO::Elf_Xword second) {
    for (auto &ac : acs) ac.swap_symbols(first, second);
  };
  // To be elf compliant, local symbols must be before all other kinds.
  symAc.arrange_local_symbols(all_swap);
}
