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
#include "core/compile/symbol/value.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_gc.hpp"
#include "core/formats/elf/managed_section_gnu_hash.hpp"
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

TEST_CASE("Section symbols", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace pepp::core::symbol;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);
  auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
  auto built = add_symtab(elf, {});

  SECTION("Nameless, local, zero-valued, and defined") {
    auto entry = built.table->section_symbol(text);
    REQUIRE(entry != nullptr);
    CHECK(entry->name.empty());
    CHECK(entry->binding == Binding::Local);
    CHECK(entry->value->type() == Type::Section);
    CHECK(entry->value->size() == 0);
    CHECK(entry->value->value()() == 0);
    CHECK(built.table->section_of(entry) == text);
    // Only exists in the ELF symtab, not the compiler's symtab
    CHECK(built.symbols->entries().empty());

    freeze_symbols(elf, built.ref);
    CHECK(built.table->frozen_order().size() == 2); // One null symbol plus section symbol
  }

  SECTION("Does not become multiply defined on sucessive calls to section_symbol") {
    auto first = built.table->section_symbol(text);
    CHECK(built.table->section_symbol(text) == first);
    CHECK(first->is_singly_defined());
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

TEST_CASE("Working with .gnu.hash", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using Binding = pepp::core::symbol::Binding;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  // Four globals and two locals. Insert globals first to ensure that sorting works.
  const auto build = [&](ManagedElf &into) {
    auto built = add_symtab(into, {"alpha", "beta", "gamma", "delta", "local_one", "local_two"});
    for (const char *name : {"alpha", "beta", "gamma", "delta"})
      built.symbols->get(name).value()->binding = Binding::Global;
    return built;
  };
  const auto hash_section_for = [&](ManagedElf &into, SectionRef symtab_ref) -> ManagedSection * {
    for (auto it = ManagedElf::SHN_UNDEF; it <= into.last_section(); ++it) {
      auto *sec = into.section(it);
      auto *hash = sec ? sec->content_as<ManagedGnuHash>() : nullptr;
      if (hash && hash->symtab() == symtab_ref) return sec;
    }
    return nullptr;
  };

  SECTION(".gnu.hash is not emitted if no hash policy is set") {
    auto built = build(elf);
    freeze_symbols(elf, built.ref);
    CHECK_FALSE(built.table->hash_parameters().has_value());
    CHECK(hash_section_for(elf, built.ref) == nullptr);
    auto ordered = built.table->frozen_order();
    REQUIRE(ordered.size() == 7);
    CHECK(ordered[3]->name == "alpha");
    CHECK(ordered[4]->name == "beta");
    CHECK(ordered[5]->name == "delta");
    CHECK(ordered[6]->name == "gamma");
  }

  SECTION("Inspect hash parameters after freeze") {
    auto built = build(elf);
    built.table->set_hash_policy({});
    freeze_symbols(elf, built.ref);
    const auto params = built.table->hash_parameters();
    REQUIRE(params.has_value());
    // Only globals are hashed, so the range starts where sh_info says the locals end.
    CHECK(params->symndx == built.table->first_nonlocal());
    CHECK(params->symndx == 3);
    CHECK(params->hashed_count == 4);
    CHECK(params->nbuckets == 2); // Four symbols at the default two per bucket.
    // 4 symbols * 12 bits/symbol = 48 bits = 2x32-bit words (rouded to power-of-2).
    CHECK(params->maskwords == 2);

    CHECK(params->shift2 == 5); // log2(32-bits) is 5

    // Ensure hash section declares symtab, strtab as dependencies
    std::vector<SectionRef> deps;
    hash_section_for(elf, built.ref)->content_as<ManagedGnuHash>()->collect_dependencies(deps);
    CHECK(deps == std::vector<SectionRef>{built.ref, elf.section(built.ref)->link});
  }

  SECTION("Hashed symbols are ordered by bucket, and locals still precede them") {
    auto built = build(elf);
    built.table->set_hash_policy({});
    freeze_symbols(elf, built.ref);
    const auto params = built.table->hash_parameters().value();
    auto ordered = built.table->frozen_order();

    // Locals keep name order ahead of the hashed range, which sh_info still marks.
    CHECK(ordered[1]->name == "local_one");
    CHECK(ordered[2]->name == "local_two");
    // The format finds a bucket's chain by position, so buckets may never decrease across the range.
    for (auto it = params.symndx + 1; it < ordered.size(); ++it) {
      INFO(ordered[it - 1]->name << " then " << ordered[it]->name);
      CHECK(pepp::djb(ordered[it - 1]->name) % params.nbuckets <= pepp::djb(ordered[it]->name) % params.nbuckets);
    }
  }

  SECTION("Re-freezing updates the section in-place") {
    auto built = build(elf);
    built.table->set_hash_policy({});
    freeze_symbols(elf, built.ref);
    const auto before = elf.section_count();
    // Dropping symbols will change the hash parameters and should cause hash section to update.
    freeze_symbols(elf, built.ref, [](const auto &e) { return e->binding == Binding::Local; });
    CHECK(elf.section_count() == before);
  }
}
