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
#include <elfio/elfio.hpp>
#include "toolchain2/asmb/pep_codegen.hpp"
#include "toolchain2/asmb/pep_ir_visitor.hpp"
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.SECTION ".text", "rwx"
bye:LDWA 10,d
.SECTION ".data", "rw"
world:.block 30
hi:.word 10
.SECTION ".text", "rwx"
cruel:BR 0
.SECTION "memvec", "rw"
World:.BYTE 0
.BYTE 0
)";
} // namespace

TEST_CASE("Pepp ASM codegen elf", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  SECTION("No ORG") {
    auto p = Parser(data(ex1));
    auto results = p.parse();
    REQUIRE(results.size() == 11);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
    auto result = pepp::tc::split_to_sections(results);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto elf_result = pepp::tc::to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab);

    CHECK(sections.size() == 3);
    elf_result.elf->save("dummy.elf");
  }
  SECTION("0-sized section") {
    auto p = Parser(data(R"(
      .SECTION ".data","rwx"
      test:BR 10,i)"));
    auto results = p.parse();
    auto result = pepp::tc::split_to_sections(results);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto elf_result = pepp::tc::to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab);

    CHECK(sections.size() == 2);
    elf_result.elf->save("dummy2.elf");
  }
}
