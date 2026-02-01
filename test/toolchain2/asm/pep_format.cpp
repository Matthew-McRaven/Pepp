
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

#include "toolchain2/asmb/pep_format.hpp"
#include <catch.hpp>
#include "core/compile/lex/buffer.hpp"
#include "toolchain2/asmb/pep_codegen.hpp"
#include "toolchain2/asmb/pep_format.hpp"
#include "toolchain2/asmb/pep_lexer.hpp"
#include "toolchain2/asmb/pep_parser.hpp"
#include "toolchain2/asmb/pep_tokens.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto idpool = []() { return std::make_shared<std::unordered_set<std::string>>(); };
static auto data = [](QString str) { return pepp::tc::support::SeekableData{str.toStdString()}; };
} // namespace

TEST_CASE("Pepp ASM source formatting", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Buffer = pepp::tc::lex::Buffer;
  using Checkpoint = pepp::tc::lex::Checkpoint;
  using Parser = pepp::tc::parser::PepParser;
  using namespace pepp::tc::lex;
  using pepp::tc::format_source;
  SECTION("Empty Line") {
    static const auto txt = "\n";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == "");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION("Comment-only") {
    static const auto txt = R"(    ;******* STRO)";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<InlineComment>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(;******* STRO)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION("Monadic Instruction") {
    {
      static const auto txt = "NOTA ;hi";
      auto l = Lexer(idpool(), data(txt));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      auto lexer_formatted = format_source(sp).toStdString();
      CHECK(lexer_formatted == "         NOTA                ;hi");
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(txt));
      auto r = p.parse(diag);
      CHECK(diag.count() == 0);
      CHECK(r.size() == 1);
      CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
    }
    {
      static const auto txt = "this: NOTA ;hi";
      auto l = Lexer(idpool(), data(txt));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      auto lexer_formatted = format_source(sp).toStdString();
      CHECK(lexer_formatted == "this:    NOTA                ;hi");
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(txt));
      auto r = p.parse(diag);
      CHECK(diag.count() == 0);
      CHECK(r.size() == 1);
      CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
    }
  }
  SECTION("Dyadic Instruction w/addressing modes") {
    {
      static const auto txt = "ADDA 15,d ;hi";
      auto l = Lexer(idpool(), data(txt));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<Identifier>());
      CHECK(b.match<pepp::tc::lex::Integer>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      auto lexer_formatted = format_source(sp).toStdString();
      CHECK(lexer_formatted == "         ADDA    15,d        ;hi");
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(txt));
      auto r = p.parse(diag);
      CHECK(diag.count() == 0);
      CHECK(r.size() == 1);
      CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
    }
    {
      static const auto txt = "this:ADDA this,sfx";
      auto l = Lexer(idpool(), data(txt));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      auto lexer_formatted = format_source(sp).toStdString();
      CHECK(lexer_formatted == "this:    ADDA    this,sfx");
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(txt));
      auto r = p.parse(diag);
      CHECK(diag.count() == 0);
      CHECK(r.size() == 1);
      CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
    }
    // Fix capitalization on addressing modes and mnemonics.
    {
      static const auto txt = "this:addA this,sFx";
      auto l = Lexer(idpool(), data(txt));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      auto lexer_formatted = format_source(sp).toStdString();
      CHECK(lexer_formatted == "this:    ADDA    this,sfx");
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(txt));
      auto r = p.parse(diag);
      CHECK(diag.count() == 0);
      CHECK(r.size() == 1);
      CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
    }
  }
  SECTION("Dyadic Instruction w/o addressing modes") {
    // Illegal assembly code which demonstrates that formatting does not depend on program being correct.
    auto l = Lexer(idpool(), data("this:ADDA this"));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<Identifier>());
    CHECK(b.match<Identifier>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    CHECK(format_source(sp).toStdString() == "this:    ADDA    this");
  }
  SECTION(".ALIGN") {
    static const auto txt = R"(execErr:   .ALIGN     8  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<DotCommand>());
    CHECK(b.match<pepp::tc::lex::Integer>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(execErr: .ALIGN  8)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".ASCII") {
    static const auto txt = R"(execErr:   .ascii "Main failed with return value \0"  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<DotCommand>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(execErr: .ASCII  "Main failed with return value \0")");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".BLOCK") {
    static const auto txt = R"(execErr:   .BLOCK     8  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<DotCommand>());
    CHECK(b.match<pepp::tc::lex::Integer>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(execErr: .BLOCK  8)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".EQUATE") {
    static const auto txt = R"(execErr:   .EQUATE     8  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<DotCommand>());
    CHECK(b.match<pepp::tc::lex::Integer>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(execErr: .EQUATE 8)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".SECTION") {
    static const auto txt = R"(.SECTION "text",    "rx")";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<DotCommand>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Literal>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(         .SECTION "text", "rx")");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".IMPORT") {
    static const auto txt = R"(.export     feed  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<DotCommand>());
    CHECK(b.match<Identifier>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(         .EXPORT feed)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
  SECTION(".ORG") {
    static const auto txt = R"(.ORG     0xfeed  )";
    auto l = Lexer(idpool(), data(txt));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<DotCommand>());
    CHECK(b.match<pepp::tc::lex::Integer>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    auto lexer_formatted = format_source(sp).toStdString();
    CHECK(lexer_formatted == R"(         .ORG    0xFEED)");
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    CHECK(format_source(r[0].get()).toStdString() == lexer_formatted);
  }
}

TEST_CASE("Pepp ASM listing formatting", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Buffer = pepp::tc::lex::Buffer;
  using Checkpoint = pepp::tc::lex::Checkpoint;
  using Parser = pepp::tc::parser::PepParser;
  using namespace pepp::tc::lex;
  using pepp::tc::format_listing;
  using pepp::tc::format_source;
  SECTION("Comment-only") {
    static const auto txt = R"(;******* STRO)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    auto result = pepp::tc::split_to_sections(diag, r);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 1);
    CHECK(listing[0].toStdString() == "            ;******* STRO");
  }
  SECTION("Monadic and Dyadic Instructions") {
    // Shows that addresses aren't always 0!
    static const auto txt = R"(this: NOTA ;hi
ADDA 15,d ;hi)";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 2);
    auto result = pepp::tc::split_to_sections(diag, r);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 1);
    CHECK(listing[0].toStdString() == "0000 1E     this:    NOTA                ;hi");
    listing = format_listing(r[1].get(), addresses, object_code);
    CHECK(listing[0].toStdString() == "0001 51000F          ADDA    15,d        ;hi");
  }
  SECTION(".BLOCK") {
    static const auto txt = R"(execErr:   .BLOCK     7  )";
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(txt));
    auto r = p.parse(diag);
    CHECK(diag.count() == 0);
    CHECK(r.size() == 1);
    auto result = pepp::tc::split_to_sections(diag, r);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto listing = format_listing(r[0].get(), addresses, object_code);
    CHECK(listing.size() == 3);
    CHECK(listing[0].toStdString() == "0000 000000 execErr: .BLOCK  7");
    CHECK(listing[1].toStdString() == "     000000");
    CHECK(listing[2].toStdString() == "     00");
  }
}
