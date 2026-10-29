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

#include "core/link/symbol_resolution.hpp"
#include <algorithm>
#include <catch.hpp>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
#include "./helpers.hpp"

namespace {
using namespace link_test;
using Elf = PackedGrowableElfFile<ElfBits::b32, ElfEndian::le>;

// An object with .text and an optional symbol table with every distinguishable binding.
Elf with_every_binding() {
  using enum SymbolBinding;
  using enum SymbolType;
  return elf_of({{"", true, STB_LOCAL, STT_SECTION},
                 {"loop", true, STB_LOCAL},
                 {"main", true, STB_GLOBAL, STT_FUNC},
                 {"helper", true, STB_WEAK, STT_FUNC},
                 {"printf", false},
                 {"optional", false, STB_WEAK}});
}

using Names = std::vector<std::string_view>;

// The names of a file's non-local symbols in [first, last).
Names names_of(const InputSymbols &table, ElfFileID file, u32 first, u32 last) {
  Names ret;
  for (u32 index = first; index < last; ++index) ret.push_back(table.name(entry_of(table)(file, index)));
  return ret;
}

// The error from gathering elf's symbols, after the caller has broken its symbol table.
DiagnosticCode error_of(Elf &elf) {
  PackedInputElfGroupLE32 group;
  REQUIRE(group.add(to_input_elf(elf)));
  Diagnostics diagnostics;
  const auto table = gather_symbols(diagnostics, group);
  REQUIRE_FALSE(table);
  REQUIRE(diagnostics.size() == 1);
  return diagnostics.begin()->code;
}


// Adds an archive of objects to group, extracting each member's ELFs
std::vector<ElfFileID> add_archive(PackedInputElfGroupLE32 &group, std::initializer_list<Symbols> members) {
  auto archive = archive_of(members);
  const auto id = group.add(to_input_archive(archive).value());
  REQUIRE(id);
  std::vector<ElfFileID> ret;
  for (const auto &member : group.archive(*id).members()) ret.push_back(group.add_member(*id, member.value()).value());
  return ret;
}
} // namespace

TEST_CASE("Collect globals / undefineds", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  PackedInputElfGroupLE32 group;
  Diagnostics diagnostics;
  auto with = with_every_binding(), without = elf_of({});
  const auto a = group.add(to_input_elf(with)).value(), b = group.add(to_input_elf(without)).value();

  SECTION("Weak, global, and undefined symbols") {
    const auto table = gather_symbols(diagnostics, group);
    REQUIRE(table);
    CHECK(table->files() == 2);
    // STN_UNDEF, then a's four non-locals, which are symbol indices 3 to 6.
    CHECK(table->size() == 5);
    CHECK(names_of(*table, a, 3, 7) == Names{"main", "helper", "printf", "optional"});
    CHECK(table->find(a, 5) == InputSymbolID{3});
    CHECK_FALSE(table->undefined(InputSymbolID{2}));
    CHECK(table->undefined(InputSymbolID{3}));
    // Index 0 is STN_UNDEF in every file, even one without a symbol table.
    CHECK(table->find(b, 0) == InputSymbolID{0});
    CHECK(table->name(InputSymbolID{0}).empty());
    CHECK(table->undefined(InputSymbolID{0}));
    // Locals have no ID, nor do indices past the end, unknown files, or files without a symbol table.
    CHECK_FALSE(table->find(a, 2));
    CHECK_FALSE(table->find(a, 7));
    CHECK_FALSE(table->find(b, 1));
    CHECK_FALSE(table->find(ElfFileID{3}, 0));
    // Each ID maps back to its file, except STN_UNDEF which doesn't belong to any file.
    CHECK(table->file_of(InputSymbolID{4}) == a);
    CHECK_THROWS_AS(table->file_of(InputSymbolID{0}), std::out_of_range);
    CHECK_THROWS_AS(table->file_of(InputSymbolID{5}), std::out_of_range);
  }
  SECTION("Reject malformed symbol tables") {
    using enum SymbolCode;
    // sh_info past the end; at loop, which is local, at 2; after main, which is global, at 3.
    const std::pair<u32, DiagnosticCode> first_globals[] = {
        {100, BadFirstGlobal}, {2, LocalAfterFirstGlobal}, {4, GlobalBeforeFirstGlobal}};
    for (const auto &[sh_info, code] : first_globals) {
      auto elf = with_every_binding();
      elf.section_headers[find_named_section(elf, ".symtab")].sh_info = sh_info;
      CHECK(error_of(elf) == code);
    }
    auto bad_link = with_every_binding();
    bad_link.section_headers[find_named_section(bad_link, ".symtab")].sh_link = 99;
    CHECK(error_of(bad_link) == DiagnosticCode{BadStringTable});
    // main, at 3, retyped as a section symbol.
    auto bad_type = with_every_binding();
    PackedSymbolWriter<ElfBits::b32, ElfEndian::le> writer(bad_type, find_named_section(bad_type, ".symtab"));
    writer.get_symbol_ptr(3)->set_type(SymbolType::STT_SECTION);
    CHECK(error_of(bad_type) == DiagnosticCode{LocalOnlyType});
  }
  SECTION("Archive symbol indices are ignored") {
    PackedGrowableArchive archive;
    const auto bytes = elf_bytes(with);
    // An incomplete symbol index in the archive, proving that symbols are read via ELF symtab not archive.
    const std::vector<std::string_view> index{"main"};
    REQUIRE(archive.add_member("a.o", std::make_shared<BlockStorage>(std::vector<char>(bytes.begin(), bytes.end())),
                               index));
    auto input = to_input_archive(archive);
    REQUIRE(input);
    REQUIRE(input->symbol_index());
    const auto member = *input->members().begin();
    REQUIRE(member);
    const auto id = group.add(std::move(*input));
    REQUIRE(id);
    const auto c = group.add_member(*id, *member);
    REQUIRE(c);
    const auto table = gather_symbols(diagnostics, group);
    REQUIRE(table);
    // helper is gathered anyway, from the member's own symbol table.
    CHECK(names_of(*table, *c, 3, 7) == names_of(*table, a, 3, 7));
    // b, which has no non-locals, starts where c does, so c's IDs must not map back to b.
    CHECK(table->file_of(*table->find(*c, 3)) == *c);
  }
}

TEST_CASE("Resolve symbols between files", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  PackedInputElfGroupLE32 group;
  // First object file's foo@V2 is explicitly versioned here, and will become the default when linked with B and become
  // foo.
  const auto a = group.add(object_of({{"foo@V2", false},
                                   {"foo", false},
                                   {"foo@V1", false},
                                   {"bar", false},
                                   {"baz@V3", false},
                                   {"qux@", false}}))
                     .value();
  const auto b = group.add(object_of({{"foo@@V2", true}, {"foo@V1", true}, {"bar", true}, {"foo@V4"}})).value();
  Diagnostics diagnostics;
  const auto table = gather_symbols(diagnostics, group);
  REQUIRE(table);
  const auto symbols = resolve_symbols(diagnostics, *table, std::vector(table->files() + 1, Liveness::Live));
  // The global symbol of file's symbol index.
  const auto of = [&, entry = entry_of(*table)](ElfFileID file, u32 index) { return symbols.of(entry(file, index)); };

  SECTION("Symbols numbered by order of first appearance") {
    // STN_UNDEF, then foo@V2(1), ending with qux@ (6). Only unique def in b is foo@V4.
    CHECK(symbols.size() == 8);
    CHECK(symbols.of(InputSymbolID{0}) == SymbolID{0});
    CHECK(of(a, 1) == SymbolID{2});
    // foo@V1 occurs in both A and B
    CHECK(of(b, 2) == SymbolID{3});
    CHECK(of(b, 4).value == SymbolID{7}.value);
  }
  SECTION("Default versions forward to the base name") {
    CHECK(symbols.name(of(b, 1)) == "foo");
    CHECK(of(a, 1) == of(b, 1));
    CHECK(of(a, 2) == of(b, 1));
  }
  SECTION("Non-default versions do not forward") {
    CHECK(symbols.name(of(a, 3)) == "foo@V1");
    CHECK(of(a, 3) == of(b, 2));
    CHECK(of(a, 3) != of(a, 2));
    // No default version of baz is defined.
    CHECK(symbols.name(of(a, 5)) == "baz@V3");
    // A bare trailing @ is part of the name, not an empty version.
    CHECK(symbols.name(of(a, 6)) == "qux@");
  }
  SECTION("Unversioned matches by name") {
    CHECK(of(a, 4) == of(b, 3));
    CHECK(symbols.name(of(a, 4)) == "bar");
  }
  SECTION("Invalid IDs") {
    CHECK_THROWS_AS(symbols.of(InputSymbolID{table->size()}), std::out_of_range);
    CHECK_THROWS_AS(symbols.name(SymbolID{symbols.size()}), std::out_of_range);
  }
}

TEST_CASE("Symbol ranking and selection", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using enum SymbolBinding;
  constexpr auto weak = STB_WEAK;
  PackedInputElfGroupLE32 group;
  // Each symbol index names the same symbol in both files.
  const auto a = group
                     .add(object_of({{.name = "weak_then_strong", .defined = true, .bind = weak},     // 1
                                     {.name = "common_then_weak", .defined = true, .common = true},   // 2
                                     {.name = "common_twice", .common = true, .size = 4, .value = 4}, // 3
                                     {.name = "strong_twice", .defined = true},                       // 4
                                     {.name = "weak_refs", .defined = false, .bind = weak},           // 5
                                     {.name = "mixed_refs", .defined = false, .bind = weak},          // 6
                                     {.name = "tls", .defined = true, .type = SymbolType::STT_TLS},   // 7
                                     {.name = "hidden", .defined = true}}))                           // 8
                     .value();
  const auto b = group.add(object_of({{.name = "weak_then_strong", .defined = true},
                                   {.name = "common_then_weak", .defined = true, .bind = weak},
                                   {.name = "common_twice", .common = true, .size = 8, .value = 2},
                                   {.name = "strong_twice", .defined = true},
                                   {.name = "weak_refs", .defined = false, .bind = weak},
                                   {.name = "mixed_refs", .defined = false},
                                   {.name = "tls", .defined = false, .type = SymbolType::STT_OBJECT},
                                   {.name = "hidden", .defined = false, .visibility = SymbolVisibility::STV_HIDDEN}}))
                     .value();
  Diagnostics diagnostics;
  const auto table = gather_symbols(diagnostics, group);
  REQUIRE(table);
  const auto symbols = resolve_symbols(diagnostics, *table, std::vector(table->files() + 1, Liveness::Live));
  const auto entry = entry_of(*table);
  const auto symbol = [&](u32 index) { return symbols.symbol(symbols.of(entry(a, index))); };

  SECTION("Strongest definition is selected as owner") {
    CHECK(symbol(1).owner == entry(b, 1));
    CHECK(symbol(2).owner == entry(b, 2));
    CHECK(symbol(4).owner == entry(a, 4));
    // Neither object actual defines 5 or 6.
    CHECK_FALSE(symbol(5).owner);
    CHECK_FALSE(symbol(6).owner);
  }
  SECTION("Common size/alignment are merged") {
    // The largest common is treated as the owner.
    CHECK(symbol(3).owner == entry(b, 3));
    CHECK(symbols.common(symbols.of(entry(a, 3))) == CommonSize{.size = 8, .alignment = 4});
    // Other definitions rank above common, which prevents size mergin.
    CHECK_FALSE(symbols.common(symbols.of(entry(a, 2))));
    CHECK(table->common(entry(a, 3)) == CommonSize{.size = 4, .alignment = 4});
    CHECK_THROWS_AS(table->common(entry(a, 4)), std::out_of_range);
  }
  SECTION("Selet most restrictive visibility") {
    CHECK(symbol(8).owner == entry(a, 8));
    CHECK(symbol(8).visibility == bits::to_underlying(SymbolVisibility::STV_HIDDEN));
  }
  SECTION("Conflicts and strong undefined references are reported in input order") {
    using enum SymbolCode;
    const auto error = Severity::Error;
    const std::vector<Diagnostic> expected{
        {error, MultipleDefinition, SymbolConflict{entry(b, 4), entry(a, 4)}},
        // weak_refs, whose references are all weak, resolves to 0 without an error.
        {error, Undefined, entry(b, 6)},
        {error, TlsMismatch, SymbolConflict{entry(b, 7), entry(a, 7)}}};
    CHECK(entries_of(diagnostics) == expected);
  }
}

TEST_CASE("Link only reachable archive members ", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  PackedInputElfGroupLE32 group;
  const auto root = group
                        .add(object_of({{.name = "needed", .defined = false},                                    // 1
                                        {.name = "optional", .defined = false, .bind = SymbolBinding::STB_WEAK}, // 2
                                        {.name = "block", .defined = true, .common = true}}))                    // 3
                        .value();
  const auto members = add_archive(
      group, {{{.name = "needed", .defined = true}, {.name = "chained", .defined = false}},
              {{.name = "chained", .defined = true}},
              {{.name = "optional", .defined = true}, {.name = "missing", .defined = false}},
              {{.name = "block", .defined = true}}});
  Diagnostics diagnostics;
  const auto table = gather_symbols(diagnostics, group);
  REQUIRE(table);
  using enum Liveness;
  // Files 1 (root)  needs 2, 3, 5, but not 4.
  const std::vector needed{Excluded, Live, Live, Live, Excluded, Live};

  std::vector files(table->files() + 1, Lazy);
  files[root.value] = Live;
  // The first pass's diagnostics are discarded, as in link_group.
  const auto link = [&] {
    Diagnostics discarded;
    mark_live_objects(*table, resolve_symbols(discarded, *table, files), files);
    std::ranges::replace(files, Lazy, Excluded);
    return resolve_symbols(diagnostics, *table, files);
  };
  const auto entry = entry_of(*table);

  SECTION("Strong/common pull in members") {
    const auto symbols = link();
    const auto owner = [&](u32 index) { return symbols.symbol(symbols.of(entry(root, index))).owner; };
    // A strong reference pulls in needed's member, whose own reference pulls in chained's.
    // A common definition pulls in a member defining the symbol.
    CHECK(files == needed);
    CHECK(owner(1) == entry(members[0], 1));
    CHECK(owner(3) == entry(members[3], 1));
    // Weak references do not pull in members and unreachables are not part of the link
    CHECK_FALSE(owner(2));
    CHECK(symbols.of(entry(members[2], 2)) == SymbolID{0});
    CHECK(diagnostics.size() == 0);
  }
  SECTION("Explicitly live members are linked") {
    files[members[2].value] = Live;
    const auto symbols = link();
    CHECK(files[members[2].value] == Live);
    CHECK(symbols.symbol(symbols.of(entry(root, 2))).owner == entry(members[2], 1));
    CHECK(entries_of(diagnostics) ==
          std::vector<Diagnostic>{{Severity::Error, SymbolCode::Undefined, entry(members[2], 2)}});
  }
}
