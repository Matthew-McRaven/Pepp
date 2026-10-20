/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <catch.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include "core/arch/pepx/asmb/pep9.hpp"
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "core/langs/asmb_pep/parser_error.hpp"
#include "core/langs/asmb_pep/text_format.hpp"

namespace {
auto data = [](std::string str) { return pepp::tc::support::SeekableData{std::move(str)}; };
} // namespace

TEST_CASE("Pep/9 dialect", "[scope:core][scope:core.langs][kind:unit][arch:pep9]") {
  using Parser = pepp::tc::parser::PepParser;
  using MR = pepp::tc::MacroRegistry;
  using PE = pepp::tc::PepParserError;
  const auto &dialect = pep::pep9::dialect();
  SECTION("opcodes specific to Pep/9 assemble") {
    static const auto src = R"(STOP
RETTR
NOP0
DECI 0x10,d
STRO 0x20,sf
NOP 5,i
NOTA
)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(src), std::make_shared<MR>(), dialect);
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    auto code = pepp::tc::parser::flatten_macros(results);
    auto split = pepp::tc::pepp_split_to_sections(diag, code);
    REQUIRE(diag.count() == 0);
    auto addresses = pepp::tc::pepp_assign_addresses(diag, split.grouped_ir);
    auto oc = pepp::tc::pepp_to_object_code(diag, addresses, split.grouped_ir);
    REQUIRE(oc.section_slices.size() == 1);
    const auto &slice = oc.section_slices[0];
    const auto bytes = slice.get(0, slice.size());
    const std::vector<u8> expected{0x00, 0x02, 0x26, 0x31, 0x00, 0x10, 0x4C, 0x00, 0x20, 0x28, 0x00, 0x05, 0x06};
    CHECK(std::vector<u8>(bytes.begin(), bytes.end()) == expected);
  }
  SECTION("Reject invalid addressing modes for traps") {
    using UE = PE::UnaryError;
    auto [source, expected] =
        GENERATE(std::make_tuple("DECI 1,i", PE::to_string(UE::AddressingMode_InvalidForMnemonic, "I")),
                 std::make_tuple("STRO 1,sx", PE::to_string(UE::AddressingMode_InvalidForMnemonic, "SX")),
                 std::make_tuple("NOP 1,d", PE::to_string(UE::AddressingMode_InvalidForMnemonic, "D")),
                 std::make_tuple("SCALL 1,i", PE::to_string(UE::Mnemonic_Invalid, "SCALL")));
    CAPTURE(source);
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(source), std::make_shared<MR>(), dialect);
    p.parse(diag);
    std::vector<std::string> messages;
    for (const auto &[_, message] : diag) messages.emplace_back(message);
    CHECK(messages == std::vector<std::string>{expected});
  }
  SECTION(".ADDRSS assembles like .WORD") {
    static const auto src = "x: .ADDRSS y\ny: .BYTE 7\n";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(src), std::make_shared<MR>(), dialect);
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    CHECK(pepp::tc::format_source(dialect, results[0].get()) == pepp::tc::format_as_columns("x:", ".ADDRSS", "y", ""));
    auto code = pepp::tc::parser::flatten_macros(results);
    auto split = pepp::tc::pepp_split_to_sections(diag, code);
    auto addresses = pepp::tc::pepp_assign_addresses(diag, split.grouped_ir);
    auto oc = pepp::tc::pepp_to_object_code(diag, addresses, split.grouped_ir);
    REQUIRE(diag.count() == 0);
    const auto &slice = oc.section_slices[0];
    const auto bytes = slice.get(0, slice.size());
    CHECK(std::vector<u8>(bytes.begin(), bytes.end()) == std::vector<u8>{0x00, 0x02, 0x07});
  }
  SECTION(".ADDRSS requires a symbol") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".ADDRSS 5\n"), std::make_shared<MR>(), dialect);
    p.parse(diag);
    std::vector<std::string> messages;
    for (const auto &[_, message] : diag) messages.emplace_back(message);
    CHECK(messages == std::vector<std::string>{PE::to_string(PE::NullaryError::Argument_ExpectedIdentifier)});
  }
  SECTION(".BURN acts like .ORG at the end of the program") {
    static const auto src = "a: .BLOCK 2\n.BURN 0xFFFF\nb: .BYTE 1\nc: .WORD 2\n";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(src), std::make_shared<MR>(), dialect);
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    CHECK(pepp::tc::format_source(dialect, results[1].get()) == pepp::tc::format_as_columns("", ".BURN", "0xFFFF", ""));
    auto code = pepp::tc::parser::flatten_macros(results);
    auto split = pepp::tc::pepp_split_to_sections(diag, code);
    REQUIRE(diag.count() == 0);
    (void)pepp::tc::pepp_assign_addresses(diag, split.grouped_ir);
    const auto address = [&](const char *name) { return p.symbol_table()->get(name).value()->value->value()(); };
    CHECK(address("a") == 0xFFFB);
    CHECK(address("b") == 0xFFFD);
    CHECK(address("c") == 0xFFFE);
    const auto &section = split.grouped_ir[0].first;
    CHECK(section.low_address == 0xFFFB);
    CHECK(section.high_address == 0x1'0000);
  }
  SECTION("At most one .BURN") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BURN 0xFFFF\n.BURN 0xFFFF\n"), std::make_shared<MR>(), dialect);
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    auto code = pepp::tc::parser::flatten_macros(results);
    pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 1);
  }
  SECTION(".BURN requires a hex argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BURN 65535\n"), std::make_shared<MR>(), dialect);
    p.parse(diag);
    std::vector<std::string> messages;
    for (const auto &[_, message] : diag) messages.emplace_back(message);
    CHECK(messages == std::vector<std::string>{PE::to_string(PE::NullaryError::Argument_ExpectedHex)});
  }
}
