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
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.SECTION ".text", "rwx"
LDWA 10,d
.SECTION ".data", "rw"
.block 30
.word 10
.SECTION ".text", "rwx"
BR 0
.SECTION "memvec", "rw"
.BYTE 0
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
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto elf = pepp::tc::to_elf(sections, addresses);
    CHECK(sections.size() == 3);
    elf->save("dummy.elf");
  }
}
