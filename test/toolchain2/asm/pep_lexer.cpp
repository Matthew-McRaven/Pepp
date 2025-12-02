
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

#include "toolchain2/asmb/pep_lexer.hpp"
#include <catch.hpp>
#include "toolchain2/asmb/pep_tokens.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto idpool = []() { return std::make_shared<pepp::tc::support::StringPool>(); };
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace
auto check_next(pepp::tc::lex::PepLexer &l, int token_type) {
  auto next = l.next_token();
  REQUIRE(next);
  CHECK(next->type() == token_type);
  return next;
}
auto check_next_string(pepp::tc::lex::PepLexer &l, int token_type, QString expected) {
  auto next = l.next_token();
  REQUIRE(next);
  CHECK(next->type() == token_type);
  CHECK(next->to_string().toStdString() == expected.toStdString());
  return next;
}
auto check_next_int(pepp::tc::lex::PepLexer &l, int64_t val, pepp::tc::lex::Integer::Format fmt) {
  auto next = l.next_token();
  REQUIRE(next);
  CHECK(next->type() == (int)pepp::tc::lex::CommonTokenType::Integer);
  auto intval = std::dynamic_pointer_cast<pepp::tc::lex::Integer>(next);
  REQUIRE(intval);
  CHECK(intval->value == val);
  CHECK(intval->format == fmt);
  return intval;
}
auto check_next_udec(pepp::tc::lex::PepLexer &l, uint64_t val) {
  return check_next_int(l, val, pepp::tc::lex::Integer::Format::UnsignedDec);
}
auto check_next_sdec(pepp::tc::lex::PepLexer &l, uint64_t val) {
  return check_next_int(l, val, pepp::tc::lex::Integer::Format::SignedDec);
}
auto check_next_hex(pepp::tc::lex::PepLexer &l, uint64_t val) {
  return check_next_int(l, val, pepp::tc::lex::Integer::Format::Hex);
}
auto check_char_sequence(pepp::tc::lex::PepLexer &l, QString body) {
  auto next = l.next_token();
  REQUIRE(next);
  CHECK(next->type() == (int)pepp::tc::lex::AsmTokenType::CharacterConstant);
  auto charconst = std::dynamic_pointer_cast<pepp::tc::lex::CharacterConstant>(next);
  REQUIRE(charconst);
  CHECK(charconst->value.toStdString() == body.toStdString());
  return charconst;
}
auto check_str_sequence(pepp::tc::lex::PepLexer &l, QString body) {
  auto next = l.next_token();
  REQUIRE(next);
  CHECK(next->type() == (int)pepp::tc::lex::AsmTokenType::StringConstant);
  auto strconst = std::dynamic_pointer_cast<pepp::tc::lex::StringConstant>(next);
  REQUIRE(strconst);
  CHECK(strconst->view().toString().toStdString() == body.toStdString());
  return strconst;
}

TEST_CASE("Pepp ASM lexer", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using CTT = pepp::tc::lex::CommonTokenType;
  using ATT = pepp::tc::lex::AsmTokenType;
  SECTION("Empty") {
    auto l = Lexer(idpool(), data("   \n  "));
    check_next(l, (int)CTT::Empty);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }
  SECTION("Comma") {
    auto l = Lexer(idpool(), data("   ,\n,  "));
    auto c = check_next(l, (int)CTT::Literal);
    CHECK(c->to_string() == ",");
    check_next(l, (int)CTT::Empty);
    c = check_next(l, (int)CTT::Literal);
    CHECK(c->to_string().toStdString() == ",");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }
  SECTION("Comment") {
    auto l = Lexer(idpool(), data(" ;Comment here\n"));
    check_next_string(l, (int)CTT::InlineComment, "Comment here");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }
  SECTION("Identifiers") {
    auto pool = idpool();
    auto l = Lexer(pool, data("a bCd b0 b9 a_word "));
    check_next_string(l, (int)CTT::Identifier, "a");
    check_next_string(l, (int)CTT::Identifier, "bCd");
    check_next_string(l, (int)CTT::Identifier, "b0");
    check_next_string(l, (int)CTT::Identifier, "b9");
    check_next_string(l, (int)CTT::Identifier, "a_word");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
    CHECK(pool->contains(u"a"));
    CHECK(!pool->contains(u"A"));
  }
  SECTION("Symbols") {
    auto pool = idpool();
    auto l = Lexer(pool, data("a: bCd: b0: b9: a_word:"));
    check_next_string(l, (int)CTT::SymbolDeclaration, "a");
    check_next_string(l, (int)CTT::SymbolDeclaration, "bCd");
    check_next_string(l, (int)CTT::SymbolDeclaration, "b0");
    check_next_string(l, (int)CTT::SymbolDeclaration, "b9");
    check_next_string(l, (int)CTT::SymbolDeclaration, "a_word");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
    CHECK(pool->contains(u"a"));
    CHECK(!pool->contains(u"a:"));
    CHECK(pool->contains(u"b9"));
    CHECK(!pool->contains(u"b9:"));
  }
  SECTION("Macro invocations") {
    auto pool = idpool();
    auto l = Lexer(pool, data("@a @bCd @b0 @b9 @a_word"));
    check_next_string(l, (int)ATT::MacroInvocation, "a");
    check_next_string(l, (int)ATT::MacroInvocation, "bCd");
    check_next_string(l, (int)ATT::MacroInvocation, "b0");
    check_next_string(l, (int)ATT::MacroInvocation, "b9");
    check_next_string(l, (int)ATT::MacroInvocation, "a_word");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
    CHECK(pool->contains(u"a"));
    CHECK(!pool->contains(u"@a"));
    CHECK(pool->contains(u"b9"));
    CHECK(!pool->contains(u"@b9"));
  }
  SECTION("Unsigned decimal") {
    auto l = Lexer(idpool(), data("0 00 000 10 65537"));
    check_next_udec(l, 0);
    check_next_udec(l, 0);
    check_next_udec(l, 0);
    check_next_udec(l, 10);
    check_next_udec(l, 65537);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }
  SECTION("Positive decimal") {
    auto l = Lexer(idpool(), data("+0 +00 +000 +10 +65537"));
    check_next_udec(l, 0);
    check_next_udec(l, 0);
    check_next_udec(l, 0);
    check_next_udec(l, 10);
    check_next_udec(l, 65537);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }
  SECTION("Negative decimal") {
    auto l = Lexer(idpool(), data("-0 -00 -000 -10 -65537"));
    check_next_sdec(l, -0);
    check_next_sdec(l, -0);
    check_next_sdec(l, -0);
    check_next_sdec(l, -10);
    check_next_sdec(l, -65537);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }

  SECTION("Sign needs digit") {
    auto l = Lexer(idpool(), data("- "));
    check_next(l, (int)CTT::Invalid);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }

  SECTION("Hexadecimal") {
    auto l = Lexer(idpool(), data("0x0 0x00 0x000 0x10 0x10000"));
    check_next_hex(l, 0);
    check_next_hex(l, 0);
    check_next_hex(l, 0);
    check_next_hex(l, 0x10);
    check_next_hex(l, 0x10000);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }

  SECTION("Hex needs digit") {
    auto l = Lexer(idpool(), data("0x"));
    check_next(l, (int)CTT::Invalid);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
  }

  SECTION("Dot") {
    auto pool = idpool();
    auto l = Lexer(pool, data(".a .bCd .b0 .b9 .a_word "));
    check_next_string(l, (int)ATT::DotCommand, "a");
    check_next_string(l, (int)ATT::DotCommand, "bCd");
    check_next_string(l, (int)ATT::DotCommand, "b0");
    check_next_string(l, (int)ATT::DotCommand, "b9");
    check_next_string(l, (int)ATT::DotCommand, "a_word");
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());
    CHECK(pool->contains(u"a"));
    CHECK(!pool->contains(u".a"));
  }

  SECTION("Dot requires char") {
    auto l = Lexer(idpool(), data("."));
    check_next(l, (int)CTT::Invalid);
    check_next(l, (int)CTT::Empty);
    CHECK(!l.input_remains());

    l = Lexer(idpool(), data(".0"));
    check_next(l, (int)CTT::Invalid);
  }

  SECTION("Unescaped chars") {
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("'H'"));
      check_char_sequence(l, "H");
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(!pool->contains(u"H"));
      CHECK(!pool->contains(u"'H'"));
    }
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("''"));
      check_next(l, (int)CTT::Invalid);
    }
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("'\"'"));
      check_char_sequence(l, "\"");
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(!pool->contains(u"\""));
      CHECK(!pool->contains(u"'\"'"));
    }
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("'aa'"));
      check_next(l, (int)CTT::Invalid);
    }
  }
  SECTION("Escaped chars") {
    QStringList escapes = {"\\n", "\\r", "\\t", "\\b", "\\\\", "\\0", "\\x7f"};
    for (const auto &s : escapes) {
      auto pool = idpool();
      auto l = Lexer(pool, data(QStringLiteral("'%1'").arg(s)));
      check_char_sequence(l, s);
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(!pool->contains(s));
    }
  }
  SECTION("Unescaped Strings") {
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("\"Hello World\""));
      check_str_sequence(l, "Hello World");
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(pool->contains(u"Hello World"));
      CHECK(!pool->contains(u"\"Hello World\""));
    }
    {
      auto pool = idpool();
      auto l = Lexer(pool, data("\"\\\"\""));
      check_str_sequence(l, "\\\"");
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(pool->contains(u"\\\""));
      CHECK(!pool->contains(u"\"\"\""));
    }
  }
  SECTION("Escaped Strings") {
    QStringList escapes = {"\\n", "\\r", "\\t", "\\b", "\\\\", "\\0", "\\x7f"};
    for (const auto &s : escapes) {
      auto pool = idpool();
      auto l = Lexer(pool, data(QStringLiteral("\"hi%1\"").arg(s)));
      check_str_sequence(l, "hi" + s);
      check_next(l, (int)CTT::Empty);
      CHECK(!l.input_remains());
      CHECK(pool->contains(u"hi" + s));
    }
  }
}
