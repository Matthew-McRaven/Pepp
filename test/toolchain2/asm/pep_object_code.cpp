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
#include "toolchain/link/mmio.hpp"
#include "toolchain2/asmb/pep_codegen.hpp"
#include "toolchain2/asmb/pep_ir_visitor.hpp"
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.SECTION ".text", "rwx"
LDWA 10,d
BR 0
.SECTION ".data", "rw"
.block 30
.SECTION "memvec", "rw"
.word 10
.BYTE 0
.BYTE 0
)";
} // namespace

TEST_CASE("Pepp ASM object code output", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;

  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(ex1));
  auto full_ir = p.parse(diag);
  CHECK(diag.count() == 0);
  auto sectioned_ir = pepp::tc::split_to_sections(full_ir);
  auto &sections = sectioned_ir.grouped_ir;
  auto addresses = pepp::tc::assign_addresses(sections);
  auto object_code = to_object_code(addresses, sections);
  CHECK(sections.size() == 3);
  CHECK(object_code.section_spans.size() == 3);
  auto s0 = object_code.section_spans[0];
  CHECK(s0.object_code.size() == 6);
  CHECK(std::vector<quint8>(s0.object_code.begin(), s0.object_code.end()) ==
        std::vector<quint8>{0xC1, 0x00, 0x0A, 0x24, 0x00, 0x00});
  auto s02 = sections[0].second[2].get();
  auto s03 = sections[0].second[3].get();
  CHECK(object_code.ir_to_object_code.find(s02) != object_code.ir_to_object_code.end());
  CHECK(object_code.ir_to_object_code.find(s03) != object_code.ir_to_object_code.end());
  CHECK(object_code.ir_to_object_code.at(s02).size() == 3);
  CHECK(object_code.ir_to_object_code.at(s03).size() == 3);
  auto s1 = object_code.section_spans[1];
  CHECK(s1.object_code.size() == 30);
  CHECK(s1.object_code[0] == 0);
  CHECK(object_code.ir_to_object_code.at(sections[1].second[1].get()).size() == 30);
  CHECK(std::equal(s1.object_code.begin() + 1, s1.object_code.end(), s1.object_code.begin()));
  auto s2 = object_code.section_spans[2];
  CHECK(s2.object_code.size() == 4);
  CHECK(std::vector<quint8>(s2.object_code.begin(), s2.object_code.end()) ==
        std::vector<quint8>{0x00, 0x0A, 0x00, 0x00});
}
