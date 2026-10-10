
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

#include "core/langs/asmb_pep/text_format.hpp"
#include <catch.hpp>
#include <fmt/format.h>
#include "core/compile/lex/buffer.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/lexer.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "spdlog/spdlog.h"

namespace {
static auto data = [](std::string str) { return pepp::tc::support::SeekableData{std::move(str)}; };
} // namespace

TEST_CASE("Pepp ASM source formatting", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Parser = pepp::tc::parser::PepParser;
  using MR = pepp::tc::MacroRegistry;
  using pepp::tc::format_source;
  SECTION("a line formats to its canonical spelling") {
    // Source on the left, the text it must format to on the right.
    using T = std::tuple<std::string, std::string>;
    auto [input, expected] = GENERATE(as<T>{},
                                      std::make_tuple("\n", ""),
                                      std::make_tuple("    ;******* STRO", ";******* STRO"),
                                      std::make_tuple("NOTA ;hi", "         NOTA                ;hi"),
                                      std::make_tuple("this: NOTA ;hi", "this:    NOTA                ;hi"),
                                      std::make_tuple("ADDA 15,d ;hi", "         ADDA    15,d        ;hi"),
                                      std::make_tuple("this:ADDA this,sfx", "this:    ADDA    this,sfx"),
                                      // Fix capitalization on addressing modes and mnemonics.
                                      std::make_tuple("this:addA this,sFx", "this:    ADDA    this,sfx"),
                                      std::make_tuple("execErr:   .ALIGN     8  ", "execErr: .ALIGN  8"),
                                      std::make_tuple(R"(execErr:   .ascii "Main failed with return value \0"  )",
                                                      R"(execErr: .ASCII  "Main failed with return value \0")"),
                                      std::make_tuple("execErr:   .BLOCK     8  ", "execErr: .BLOCK  8"),
                                      std::make_tuple("execErr:   .EQUATE     8  ", "execErr: .EQUATE 8"),
                                      std::make_tuple(R"(.SECTION "text",    "rx")", R"(         .SECTION "text", "rx")"),
                                      std::make_tuple(".export     feed  ", "         .EXPORT feed"),
                                      std::make_tuple(".ORG     0xfeed  ", "         .ORG    0xFEED"));
    CAPTURE(input);
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(input), std::make_shared<MR>());
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(r.size() == 1);
    CHECK(format_source(r[0].get()) == expected);
  }
  SECTION("Macro Definition") {
    // Bodies are not formatted; they are reproduced as written.
    static const auto txt =
        R"(.MACRO my_macro arg1, arg2
.BYTE 0
.ENDM
)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt), std::make_shared<MR>());
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    for (const auto &d : diag) SPDLOG_WARN(" {}", d.second);
    CHECK(r.size() == 1);
    auto source = format_source(r[0].get());

    CHECK(source ==
          R"(         .MACRO my_macro arg1, arg2
.BYTE 0
         .ENDM)");
  }
  SECTION("Macro Instance") {
    static const auto txt = R"(execErr:   my_macro  ;comment)";
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto md = std::make_shared<pepp::tc::MacroDefinition>();
    md->name = "my_macro";
    md->body = "";
    CHECK(mr->insert(md));
    auto p = Parser(data(txt), mr);
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    auto source = format_source(r[0].get());
    CHECK(source == R"(execErr: my_macro             ;comment)");
  }
}

TEST_CASE("Pepp ASM listing formatting",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Buffer = pepp::tc::lex::Buffer;
  using Checkpoint = pepp::tc::lex::Checkpoint;
  using Parser = pepp::tc::parser::PepParser;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc::lex;
  using pepp::tc::format_listing;
  using pepp::tc::format_source;
  SECTION("Comment-only") {
    static const auto txt = R"(;******* STRO)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt), std::make_shared<MR>());
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    auto code = pepp::tc::parser::flatten_macros(r);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 1);
    CHECK(listing[0] == "            ;******* STRO");
  }
  SECTION("Monadic and Dyadic Instructions") {
    // Shows that addresses aren't always 0!
    static const auto txt = R"(this: NOTA ;hi
ADDA 15,d ;hi)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt), std::make_shared<MR>());
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 2);
    auto code = pepp::tc::parser::flatten_macros(r);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 1);
    CHECK(listing[0] == "0000 1E     this:    NOTA                ;hi");
    listing = format_listing(r[1].get(), addresses, object_code);
    CHECK(listing[0] == "0001 51000F          ADDA    15,d        ;hi");
  }
  SECTION(".BLOCK") {
    static const auto txt = R"(execErr:   .BLOCK     7  )";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt), std::make_shared<MR>());
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    auto code = pepp::tc::parser::flatten_macros(r);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 3);
    CHECK(listing[0] == "0000 000000 execErr: .BLOCK  7");
    CHECK(listing[1] == "     000000");
    CHECK(listing[2] == "     00");
  }
}

TEST_CASE("Pepp ASM symbol-only line formatting",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Parser = pepp::tc::parser::PepParser;
  using MR = pepp::tc::MacroRegistry;
  // Symbol-only lines (with and without a comment) before code with a trailing label.
  static const auto txt = "a: ;hi\nb:\nLDWA 10,d\nend:";
  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(txt), std::make_shared<MR>());
  auto r = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(r.size() == 4);

  const auto source = pepp::tc::format_source(r);
  const std::vector<std::string> expected_source = {"a:" + std::string(27, ' ') + ";hi", "b:", "         LDWA    10,d",
                                                    "end:"};
  CHECK(source == expected_source);

  auto code = pepp::tc::parser::flatten_macros(r);
  auto result = pepp::tc::pepp_split_to_sections(diag, code);
  CHECK(diag.count() == 0);
  auto &sections = result.grouped_ir;
  auto addresses = pepp::tc::pepp_assign_addresses(sections);
  auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
  const auto bare = [](const std::string &text) { return fmt::format("{:12}{}", "", text); };
  const std::vector<std::string> expected_listing = {bare(expected_source[0]), bare(expected_source[1]),
                                                     "0000 C1000A " + expected_source[2], bare(expected_source[3])};
  CHECK(pepp::tc::format_listing(sections[0].second, addresses, object_code) == expected_listing);
}

TEST_CASE("Pepp ASM symbol + value operands", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Parser = pepp::tc::parser::PepParser;
  using MR = pepp::tc::MacroRegistry;
  // Expressions naming labels need relocations, which are not supported yet; constants are.
  static const auto txt = "ADDA 0x0F + 1,i\n"   // 0x0000, hex + decimal
                          "BR end\n"            // 0x0003, plain symbol operands still work
                          "val: .EQUATE 10 + 5\n"
                          "LDWA val+1,i\n"      // 0x0006, constant symbol + value
                          "end: NOTA";           // 0x0009
  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(txt), std::make_shared<MR>());
  auto r = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(r.size() == 5);
  auto code = pepp::tc::parser::flatten_macros(r);
  auto result = pepp::tc::pepp_split_to_sections(diag, code);
  CHECK(diag.count() == 0);
  auto &sections = result.grouped_ir;
  auto addresses = pepp::tc::pepp_assign_addresses(sections);
  auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);

  // Expressions format with spaces around the operator, and each row shows the bytes that were generated.
  const auto row = [](u16 address, const char *bytes, const char *source) {
    return fmt::format("{:04X} {:<6} {}", address, bytes, source);
  };
  const std::vector<std::string> expected = {
      row(0x0, "500010", "         ADDA    0xF + 1,i"),
      row(0x3, "240009", "         BR      end"),
      fmt::format("{:12}{}", "", "val:     .EQUATE 10 + 5"), // Generates no bytes and has no address.
      row(0x6, "C00010", "         LDWA    val + 1,i"),
      row(0x9, "1E", "end:     NOTA"),
  };
  CHECK(pepp::tc::format_listing(sections[0].second, addresses, object_code) == expected);
}

