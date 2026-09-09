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

#include "core/formats/elf/managed_section_reloc.hpp"
#include <catch.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_symtab.hpp"
#include "core/math/bitmanip/enums.hpp"

namespace {
using namespace pepp::bts;
using LeafTable = pepp::core::symbol::LeafTable;
} // namespace

TEST_CASE("Relocation tables", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);
  auto symbols = std::make_shared<LeafTable>(2);
  auto *sec = elf.section(elf.add_section(".rela.text", SectionTypes::SHT_RELA));
  auto &table = sec->make_content<ManagedRelocTable>();

  SECTION("Entries are kept in the order they were written") {
    auto first = symbols->reference("first"), second = symbols->reference("second");
    table.add({.offset = 4, .symbol = first, .type = 1, .addend = 0});
    table.add({.offset = 1, .symbol = second, .type = 1, .addend = 7});
    auto entries = table.entries();
    REQUIRE(entries.size() == 2);
    CHECK(entries[0].symbol == first);
    CHECK(entries[0].offset == 4);
    CHECK(entries[1].symbol == second);
    CHECK(entries[1].addend == 7);
  }

  SECTION("Relocation name symbols") {
    CHECK_THROWS_AS(table.add({.offset = 0, .symbol = nullptr, .type = 1}), std::logic_error);
    CHECK(table.entries().empty());
  }

  SECTION("Size matches the target's word size") {
    table.add({.offset = 0, .symbol = symbols->reference("only"), .type = 1});
    // Per ELF TIS figure 1-21: r_offset, r_info and r_addend, each one word wide.
    CHECK(sec->file_bytes() == 12);
    ManagedElf wide(ElfBits::b64, ElfEndian::le, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);
    auto *sec64 = wide.section(wide.add_section(".rela.text", SectionTypes::SHT_RELA));
    sec64->make_content<ManagedRelocTable>().add({.offset = 0, .symbol = symbols->reference("only"), .type = 1});
    CHECK(sec64->file_bytes() == 24);
  }
}

TEST_CASE("Finding a section's relocation table", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace bits;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);
  auto symbols = std::make_shared<LeafTable>(2);
  auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
  auto symtab = elf.add_section(".symtab", SectionTypes::SHT_SYMTAB);
  elf.section(symtab)->make_content<ManagedSymbolTable>(symbols);

  // The section holding relocations for `target`, or null if none was made.
  const auto rela_for = [&](SectionRef target) -> ManagedSection * {
    for (auto it = ManagedElf::SHN_UNDEF; it <= elf.last_section(); ++it) {
      auto *sec = elf.section(it);
      if (!sec || !sec->content_as<ManagedRelocTable>()) continue;
      else if (std::get_if<SectionRef>(&sec->info) && std::get<SectionRef>(sec->info) == target) return sec;
    }
    return nullptr;
  };

  SECTION("Created on first use") {
    auto &table = relocations_for(elf, text, symtab);
    auto *sec = rela_for(text);
    REQUIRE(sec != nullptr);
    CHECK(sec->name == ".rela.text");
    CHECK(sec->type == SectionTypes::SHT_RELA);
    CHECK(sec->link == symtab);
    CHECK(sec->addralign == 4);
    CHECK(bits::to_underlying(sec->flags & SectionFlags::SHF_INFO_LINK) != 0);
    CHECK(sec->content_as<ManagedRelocTable>() == &table);
  }

  SECTION("At most one .RELA per section") {
    auto &first = relocations_for(elf, text, symtab);
    first.add({.offset = 0, .symbol = symbols->reference("charin"), .type = 1});
    const auto before = elf.section_count();
    auto &again = relocations_for(elf, text, symtab);
    CHECK(&again == &first);
    CHECK(elf.section_count() == before);
    // The one already holding entries, not a fresh one alongside it.
    CHECK(again.entries().size() == 1);
  }

  SECTION("Each relocated section gets a unique instane") {
    auto data = elf.add_section(".data", SectionTypes::SHT_PROGBITS);
    auto &for_text = relocations_for(elf, text, symtab);
    auto &for_data = relocations_for(elf, data, symtab);
    CHECK(&for_text != &for_data);
    CHECK(rela_for(data)->name == ".rela.data");
  }

  SECTION("Refuses an unusable target or symbol table") {
    CHECK_THROWS_AS(relocations_for(elf, SectionRef{999}, symtab), std::logic_error);
    CHECK_THROWS_AS(relocations_for(elf, text, text), std::logic_error);
  }
}
