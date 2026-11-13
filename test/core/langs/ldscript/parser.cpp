/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include "core/langs/ldscript/parser.hpp"
#include <array>
#include <catch.hpp>
#include <memory>
#include <span>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/expr/lexer.hpp"
#include "core/langs/expr/parser.hpp"
#include "core/langs/ldscript/expression.hpp"
#include "core/langs/ldscript/parser_error.hpp"

namespace {
using namespace pepp::tc::ldscript;
using pepp::tc::support::Location;
using pepp::tc::support::LocationInterval;
using pepp::tc::support::SeekableData;
namespace expr = pepp::tc::expr;

// Parse text that must not produce any diagnostics.
Script require_parse(std::string_view text) {
  pepp::tc::DiagnosticTable diag;
  auto ret = parse(diag, text);
  for (auto it = diag.cbegin(); it != diag.cend(); ++it)
    UNSCOPED_INFO(it->second.message << " at " << it->first.lower().row << ":" << it->first.lower().column);
  REQUIRE(diag.count() == 0);
  return ret;
}

std::string parse_then_format(std::string_view text) { return to_string(require_parse(text)); }

// Compare here, since Catch cannot print a LocationInterval.
bool spans(const LocationInterval &interval, Location lower, Location upper) {
  return interval.lower() == lower && interval.upper() == upper;
}

struct Reported {
  Location at;
  std::string message;
};

// Returns the diagnostics in diag, in source order.
std::vector<Reported> reported(const pepp::tc::DiagnosticTable &diag) {
  std::vector<Reported> ret;
  for (auto it = diag.cbegin(); it != diag.cend(); ++it) ret.emplace_back(it->first.lower(), it->second.message);
  return ret;
}

// Return the repr() of each token that the expression lexer produces up to EoF.
std::vector<std::string> reprs(std::string text) {
  expr::ExpressionLexer lexer(std::make_shared<std::unordered_set<std::string>>(), SeekableData(std::move(text)),
                              expression_options());
  std::vector<std::string> ret;
  do {
    ret.push_back(lexer.next_token()->repr());
    REQUIRE(ret.size() < 100); // Fail instead of looping forever if the lexer stops advancing.
  } while (ret.back() != "EoF()");
  return ret;
}

expr::Type first(std::span<const expr::Type> types, expr::Type) { return types[0]; }
constexpr std::array one_value{expr::Function::Parameter::Value};
constexpr std::array two_values{expr::Function::Parameter::Value, expr::Function::Parameter::Value};
// Overloads of ld's ALIGN, chosen by arity (e.g., ALIGN(align), ALIGN(exp, align)).
const std::array<expr::Function, 2> align{{
    {"ALIGN", expr::Function::Category::NonConstExpr, one_value, nullptr, first},
    {"ALIGN", expr::Function::Category::NonConstExpr, two_values, nullptr, first},
}};
} // namespace

TEST_CASE("ld script expressions", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using V = std::vector<std::string>;
  SECTION("Words") {
    // Each word is lexed whole, as a number, or as a name unless it starts with a digit.
    CHECK(reprs("$10K 1bh 12b") == V{"Integer(0x4000)", "Integer(0x1B)", "Invalid(12b)", "EoF()"});
    CHECK(reprs("bad fade . .text a.b __global_pointer$ _start-2") ==
          V{"Identifier(bad)", "Identifier(fade)", "Literal(.)", "Identifier(.text)", "Identifier(a.b)",
            "Identifier(__global_pointer$)", "Identifier(_start)", "Literal(-)", "Integer(2)", "EoF()"});
    CHECK(reprs("'a'") == V{"Invalid(')", "Identifier(a)", "Invalid(')", "EoF()"});
    // An unclosed comment is invalid.
    CHECK(reprs("a /* x") == V{"Identifier(a)", "Invalid(/* x)", "EoF()"});
  }
  SECTION("Expressions span lines and comments") {
    auto options = expression_options();
    options.functions = align;
    const expr::NameLocationCounter name = [] { return std::string("."); };
    const auto parse_ld = [&](const char *source) {
      return expr::parse(source, Location(0, 0), nullptr, options, name);
    };
    const auto multiline = parse_ld("ALIGN(. + 4K,\n /* x */ 8) - .text;\nnext");
    REQUIRE(std::holds_alternative<expr::Parsed>(multiline));
    const auto &result = std::get<expr::Parsed>(multiline);
    CHECK(expr::to_postfix(result.tree) == ". 4096 + 8 ALIGN .text -");
    CHECK(result.length == 33);
    CHECK(result.after.rest() == ";\nnext");
    CHECK(result.after.location() == Location(1, 19));
    // The overload is chosen by arity.
    const auto one = parse_ld("ALIGN(4)"), two = parse_ld("ALIGN(., 4)"), three = parse_ld("ALIGN(1, 2, 3)");
    const auto &one_tree = std::get<expr::Parsed>(one).tree, &two_tree = std::get<expr::Parsed>(two).tree;
    CHECK(std::get<expr::Call>(one_tree[one_tree.root()]).function == &align[0]);
    CHECK(std::get<expr::Call>(two_tree[two_tree.root()]).function == &align[1]);
    REQUIRE(std::holds_alternative<expr::Error>(three));
    CHECK(std::get<expr::Error>(three).matches(expr::UnaryError::Function_WrongArity, "ALIGN"));
  }
}

TEST_CASE("ld script parser", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  SECTION("Round-trips through parser") {
    // Uses comments, stray ;, = and : without spaces, commas between names, and quoted names.
    const auto lenient = R"(/* RV */ OUTPUT_FORMAT(elf32-littleriscv, "elf32-bigriscv", elf32-littleriscv)
OUTPUT_ARCH(riscv) ENTRY(_start); # to end of line
INPUT(a.o, -lc) GROUP(libx.a libc.a)
x=4;
SECTIONS{
  . = 0x200;
  .text # address
  0x1000: { *(.text, .text.*) ; KEEP(crt0.o(.init)) y = . + 4K; }
  ".data" : { PROVIDE(z = .) "b.o"(.data) lib.a:m.o(.data) KEEP(lib.a:(.init)) :c.o(.data) }
  "a b": { *("x:y") }
  w += 1;
}
)";
    const auto strict = R"(OUTPUT_FORMAT("elf32-littleriscv", "elf32-bigriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)
INPUT(a.o -lc)
GROUP(libx.a libc.a)
x = 4;
SECTIONS
{
  . = 0x200;
  .text 0x1000 :
  {
    *(.text .text.*)
    KEEP(crt0.o(.init))
    y = . + 4096;
  }
  .data :
  {
    PROVIDE(z = .);
    b.o(.data)
    "lib.a:m.o"(.data)
    KEEP(lib.a:(.init))
    :c.o(.data)
  }
  "a b" :
  {
    *("x:y")
  }
  w += 1;
}
)";
    const auto script = require_parse(lenient);
    CHECK(to_string(script) == strict);
    CHECK(parse_then_format(strict) == strict);
    const auto &commands = script.commands;
    const auto &format = std::get<OutputFormat>(commands[0]);
    CHECK(format.big.encoding == pepp::bts::ElfEncoding::ELFDATA2MSB);
    CHECK(std::get<OutputArch>(commands[1]).machine == pepp::bts::ElfMachineType::EM_RISCV);
    CHECK(!std::get<Assignment>(commands[5]).compound);
    const auto &last = std::get<Sections>(commands[6]).commands.back();
    CHECK(std::get<Assignment>(last).compound == pepp::tc::expr::BinaryOp::Add);
  }
  SECTION("Source intervals recorded on IR") {
    const auto script = require_parse("ENTRY(_start)\nSECTIONS { .text : { KEEP(*(.init)) } x = 1 + 2; }");
    const auto &commands = script.commands;
    const auto &entry = std::get<Entry>(commands[0]);
    CHECK(spans(entry.source_interval, Location(0, 0), Location(0, 13)));
    const auto &inner = std::get<Sections>(commands[1]).commands;
    const auto &keep = std::get<InputSectionDescription>(std::get<OutputSectionDescription>(inner[0]).commands[0]);
    CHECK(spans(keep.source_interval, Location(1, 21), Location(1, 35)));
    const auto &assignment = std::get<Assignment>(inner[1]);
    CHECK(spans(assignment.source_interval, Location(1, 38), Location(1, 48)));
  }
  SECTION("Failed parsing does not halt parsing") {
    using NullaryError = ParserError::NullaryError;
    using UnaryError = ParserError::UnaryError;
    // Each line is a differnt form of failure. OUTPUT_FORMAT needs a ; because synchronize() looks for a } or ;.
    const auto text = R"(OUTPUT_FORMAT(elf32-unknown);
ENTRY(_start)
x = (1;
SECTIONS { .text : { *(.text } }
y = 2;
z = 4)";
    pepp::tc::DiagnosticTable diag;
    const auto script = parse(diag, text);
    CHECK(to_string(script) == "ENTRY(_start)\ny = 2;\n");
    const auto errors = reported(diag);
    REQUIRE(errors.size() == 4);
    CHECK(errors[0].message == ParserError::to_string(UnaryError::Format_Unknown, "elf32-unknown"));
    CHECK(errors[0].at == Location(0, 14));
    CHECK(errors[1].message.starts_with("Invalid expression"));
    CHECK(errors[1].at.row == 2);
    CHECK(errors[2].message == ParserError::to_string(NullaryError::Name_Expected));
    CHECK(errors[2].at.row == 3);
    CHECK(errors[3].message == ParserError::to_string(UnaryError::Token_Expected, ";"));
    CHECK(errors[3].at == Location(5, 5));
  }
}
