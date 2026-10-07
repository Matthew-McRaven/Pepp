/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>
#include <map>
#include <sstream>
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "elfio/elfio.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("RISCV ASM code generator",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Lexer = pepp::langs::RISCVLexer;
  using Parser = pepp::tc::parser::RISCVParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  SECTION("R Type: add x1, x2, x3") { // matches sources of samples add.S
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("add x1, x2, x3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto result = pepp::tc::riscv_split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0xfeed);
    CHECK(addresses.container.size() == 1);
    auto instr = std::dynamic_pointer_cast<RTypeIR>(sections[0].second[0]);
    CHECK(instr != nullptr);
    CHECK(instr->mnemonic.mn == riscv::ADD);
    CHECK(addresses.count(&*instr) == 1);
    CHECK(addresses.at(&*instr).address == 0xfeed);
    CHECK(addresses.at(&*instr).size == 4);
    auto object_code = pepp::tc::riscv_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::riscv_to_elf(sections, addresses, object_code, *symbol_tab);
    // Read what would actually be written, rather than the in-memory model of it.
    const auto bytes = pepp::tc::elf_bytes(elf_result);
    std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    ELFIO::elfio elf;
    REQUIRE(elf.load(in));
    REQUIRE(elf.sections[".text"] != nullptr);
    CHECK(elf.sections[".text"]->get_size() == 4);
    // Per samples directory, bytes should be little-endian b3 00 31 00
    auto text_section = elf.sections[".text"];
    auto data = text_section->get_data();
    CHECK((u8)data[0] == 0xb3);
    CHECK((u8)data[1] == 0x00);
    CHECK((u8)data[2] == 0x31);
    CHECK((u8)data[3] == 0x00);
  }
}

namespace {
struct Expect {
  const char *name;
  u64 address, size;
  pepp::core::symbol::Type type;
};
} // namespace
TEST_CASE("RISCV ASM code generator symbol-only lines",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Parser = pepp::tc::parser::RISCVParser;
  using Type = pepp::core::symbol::Type;
  using namespace pepp::tc;
  using Symbols = std::map<std::string, std::shared_ptr<pepp::core::symbol::Entry>>;
  // Assemble, assign addresses, and convert to object code, returning all symbol declarations.
  auto assemble = [](const char *source, u32 &total_size) {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(source));
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    auto split = pepp::tc::riscv_split_to_sections(diag, results);
    REQUIRE(diag.count() == 0);
    auto &sections = split.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0x100);
    total_size = 0;
    for (const auto &pair : addresses.container) total_size += pair.second.size;
    (void)pepp::tc::riscv_to_object_code(addresses, sections);
    Symbols ret;
    for (const auto &line : results)
      if (auto sym = line->typed_attribute<SymbolDeclaration>(); sym) ret[std::string(sym->entry->name)] = sym->entry;
    return ret;
  };

  const auto check = [](const Symbols &symbols, const std::vector<Expect> &expected) {
    REQUIRE(symbols.size() == expected.size());
    for (const auto &e : expected) {
      CAPTURE(e.name);
      REQUIRE(symbols.count(e.name) == 1);
      const auto &value = *symbols.at(e.name)->value;
      CHECK(value.value()() == e.address);
      CHECK(value.size() == e.size);
      CHECK(value.type() == e.type);
    }
  };

  SECTION("symbols inherit their target's location") {
    u32 size = 0;
    // Sections run A B A, with labels at the tail of the first A and code after them in the second A.
    auto symbols = assemble(".SECTION \".text\", \"rwx\"\n"
                            "add x1, x2, x3\n"      // 0x100
                            "a:\nb:\n# comment\n\n" // a, b: target the sub, across comments and blanks
                            "sub x1, x2, x3\n"      // 0x104
                            "c:\n"                  // alias of d
                            "d: add x1, x2, x3\n"   // 0x108
                            "f:\ng:\n"              // tail of A; must not be moved to the end of A
                            ".SECTION \".data\", \"rw\"\n"
                            "e:\n"      // target is data, not code
                            ".WORD 5\n" // 0x110
                            "h:\n"      // tail of B
                            ".SECTION \".text\", \"rwx\"\n"
                            "sub x1, x2, x3\n", // 0x10c, after the tail labels
                            size);
    check(symbols, {{"a", 0x104, 4, Type::Code},
                    {"b", 0x104, 4, Type::Code},
                    {"c", 0x108, 4, Type::Code},
                    {"d", 0x108, 4, Type::Code},
                    {"f", 0x10c, 0, Type::Object},
                    {"g", 0x10c, 0, Type::Object},
                    {"e", 0x110, 4, Type::Object},
                    {"h", 0x114, 0, Type::Object}});
    // Labels share a location but remain distinct symbols; one before a labeled line aliases it.
    CHECK(symbols.at("a") != symbols.at("b"));
    CHECK(dynamic_cast<pepp::core::symbol::AliasValue *>(symbols.at("c")->value.get()));
    CHECK(!dynamic_cast<pepp::core::symbol::AliasValue *>(symbols.at("d")->value.get()));
    CHECK(size == 20); // Symbol-only lines occupy no space.
  }
  SECTION("symbols before .ORG") {
    u32 size = 0;
    auto symbols = assemble("add x1, x2, x3\n.ORG 0x200\nsub x1, x2, x3\nfoo:\n.ORG 0x400\nadd x1, x2, x3", size);
    check(symbols, {{"foo", 0x204, 0, Type::Object}});
  }
}
