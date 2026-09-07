#include "core/formats/elf/managed_section_shstrtab.hpp"
#include "core/formats/elf/enums.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"

pepp::bts::SectionRef pepp::bts::build_shstrtab(ManagedElf &elf, std::vector<SectionRef> &live) {
  auto ref = elf.shstrtab();
  if (!elf.section(ref)) {
    ref = elf.add_section(".shstrtab", SectionTypes::SHT_STRTAB);
    elf.set_shstrtab(ref);
  }
  // Live list might predate shstrab being created, so we need to add it if not present.
  if (std::find(live.begin(), live.end(), ref) == live.end()) live.push_back(ref);

  // Force this section to become a string table.
  auto *sec = elf.section(ref);
  if (!std::holds_alternative<ManagedStringTable>(sec->content)) sec->content.emplace<ManagedStringTable>();
  auto &table = std::get<ManagedStringTable>(sec->content);
  for (auto member : live)
    if (const auto *named = elf.section(member); named) table.insert(named->name);
  // Force clear link and info because they are meaningless for a string table.
  sec->link = ManagedElf::SHN_UNDEF, sec->info = u32{0};
  return ref;
}