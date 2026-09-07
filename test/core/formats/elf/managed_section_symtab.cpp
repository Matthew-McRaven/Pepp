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

#include "core/formats/elf/managed_section_symtab.hpp"
#include <algorithm>
#include <catch.hpp>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_gc.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"

namespace {
using namespace pepp::bts;
using LeafTable = pepp::core::symbol::LeafTable;

// A .symtab holding names, with no sh_link yet. Pointer size is irrelevant but must be set.
struct Built {
  ManagedSection *sec = nullptr;
  SectionRef ref;
  std::shared_ptr<LeafTable> symbols = std::make_shared<LeafTable>(2);
  ManagedSymbolTable *table = nullptr;
};

Built add_symtab(ManagedElf &elf, std::initializer_list<const char *> names,
                 SectionTypes type = SectionTypes::SHT_SYMTAB) {
  Built ret;
  ret.ref = elf.add_section(type == SectionTypes::SHT_DYNSYM ? ".dynsym" : ".symtab", type);
  ret.sec = elf.section(ret.ref);
  for (const char *name : names) ret.symbols->define(name);
  ret.table = &ret.sec->make_content<ManagedSymbolTable>(ret.symbols);
  return ret;
}
} // namespace

TEST_CASE("Freeze symbol tables", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  SECTION("Sort locals before globals then alphabetically") {
    auto built = add_symtab(elf, {"zeta", "alpha", "beta"});
    built.symbols->get("alpha").value()->binding = pepp::core::symbol::Binding::Global;
    freeze_symbols(elf, built.ref);
    auto ordered = built.table->frozen_order();
    REQUIRE(ordered.size() == 4);
    // Index 0 is the reserved null symbol, which carries no entry.
    CHECK(ordered[0] == nullptr);
    CHECK(ordered[1]->name == "beta");
    CHECK(ordered[2]->name == "zeta");
    CHECK(ordered[3]->name == "alpha");
    CHECK(built.table->first_nonlocal() == 3);
    CHECK(std::get<u32>(built.sec->info) == 3);
  }

  SECTION("Predicates determine live symbols") {
    auto built = add_symtab(elf, {"kept", "dropped"});
    auto dropped = built.symbols->get("dropped").value();
    freeze_symbols(elf, built.ref, [&](const auto &entry) { return entry != dropped; });
    auto ordered = built.table->frozen_order();
    REQUIRE(ordered.size() == 2);
    CHECK(ordered[1]->name == "kept");
    CHECK(built.table->first_nonlocal() == 2);
  }

  SECTION("Freeze can be applied multiple times") {
    auto built = add_symtab(elf, {"a", "b"});
    freeze_symbols(elf, built.ref);
    CHECK(built.table->frozen_order().size() == 3);
    freeze_symbols(elf, built.ref, [](const auto &) { return false; });
    // Only the null symbol survives, and every table has one.
    CHECK(built.table->frozen_order().size() == 1);
  }

  SECTION("Unfrozen symbol table throws for most operations") {
    auto built = add_symtab(elf, {"main"});
    CHECK_FALSE(built.table->is_frozen());
    CHECK_THROWS_AS(built.table->frozen_order(), std::logic_error);
    CHECK_THROWS_AS(built.table->first_nonlocal(), std::logic_error);
    CHECK_THROWS_AS(built.table->file_bytes(ElfBits::b32), std::logic_error);
  }
}

TEST_CASE("Build string tables for symbol tables", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  SECTION("Symbol table without sh_link will have a strtab added") {
    auto built = add_symtab(elf, {"main", "exit"});
    CHECK_FALSE(elf.section(built.ref)->link);
    freeze_symbols(elf, built.ref);

    auto *symtab = elf.section(built.ref);
    REQUIRE(symtab->link);
    auto *strtab = elf.section(symtab->link);
    REQUIRE(strtab != nullptr);
    CHECK(strtab->name == ".strtab");
    CHECK(strtab->type == SectionTypes::SHT_STRTAB);

    auto live = garbage_collect_sections(elf);
    CHECK(std::find(live.begin(), live.end(), symtab->link) != live.end());
    build_strtabs_for_symtabs(elf, live);

    const auto *names = strtab->content_as<ManagedStringTable>();
    REQUIRE(names != nullptr);
    CHECK(names->find("main").has_value());
    CHECK(names->find("exit").has_value());
  }

  SECTION("A dynamic symbol table gets .dynstr instead") {
    auto built = add_symtab(elf, {"main"}, SectionTypes::SHT_DYNSYM);
    freeze_symbols(elf, built.ref);
    auto live = garbage_collect_sections(elf);
    build_strtabs_for_symtabs(elf, live);
    CHECK(elf.section(elf.section(built.ref)->link)->name == ".dynstr");
  }

  SECTION("Existing string tables are filled rather than replaced") {
    auto built = add_symtab(elf, {"main"});
    auto strtab = elf.add_section(".strtab", SectionTypes::SHT_STRTAB);
    elf.section(strtab)->make_content<ManagedStringTable>().insert("already here");
    elf.section(built.ref)->link = strtab;
    const auto before = elf.section_count();
    freeze_symbols(elf, built.ref);
    auto live = garbage_collect_sections(elf);
    build_strtabs_for_symtabs(elf, live);

    CHECK(elf.section(built.ref)->link == strtab);
    CHECK(elf.section_count() == before);
    const auto *names = elf.section(strtab)->content_as<ManagedStringTable>();
    REQUIRE(names != nullptr);
    CHECK(names->find("already here").has_value());
    CHECK(names->find("main").has_value());
  }

  SECTION("Only frozen symbols are interned") {
    auto built = add_symtab(elf, {"kept", "dropped"});
    auto dropped = built.symbols->get("dropped").value();
    freeze_symbols(elf, built.ref, [&](const auto &entry) { return entry != dropped; });
    auto live = garbage_collect_sections(elf);
    build_strtabs_for_symtabs(elf, live);

    const auto *names = elf.section(elf.section(built.ref)->link)->content_as<ManagedStringTable>();
    REQUIRE(names != nullptr);
    CHECK(names->find("kept").has_value());
    CHECK_FALSE(names->find("dropped").has_value());
  }

  SECTION("Garbage-collected symbol tables do not create string tables") {
    auto built = add_symtab(elf, {"main"});
    auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    elf.section(text)->content.emplace<RawBytes>().bytes = {1, 2, 3};
    std::vector<SectionRef> live{text}; // Leave symtab out of live set.
    const auto before = elf.section_count();
    build_strtabs_for_symtabs(elf, live);
    CHECK(elf.section_count() == before);
    CHECK_FALSE(elf.section(built.ref)->link);
    CHECK_FALSE(elf.section(text)->link);
  }
}

TEST_CASE("Building string tables refuses a malformed symbol table", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  SECTION("An sh_link pointing to an invalid value") {
    auto built = add_symtab(elf, {"main"});
    freeze_symbols(elf, built.ref);
    elf.section(built.ref)->link = SectionRef{999};
    auto live = garbage_collect_sections(elf);
    CHECK_THROWS_AS(build_strtabs_for_symtabs(elf, live), std::logic_error);
  }

  SECTION("An sh_link pointing to a non-stringtable") {
    auto built = add_symtab(elf, {"main"});
    freeze_symbols(elf, built.ref);
    elf.section(built.ref)->link = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    auto live = garbage_collect_sections(elf);
    CHECK_THROWS_AS(build_strtabs_for_symtabs(elf, live), std::logic_error);
  }
}
