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
#include "antlr4-runtime.h"

#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"
#include "toolchain/pas/parse/pepp/PeppASTConverter10.h"
#include "isa/pep10.hpp"

#include "toolchain/parse/PeppLexer.h"
#include "toolchain/parse/PeppLexerErrorListener.h"
#include "toolchain/parse/PeppParser.h"

using namespace antlr4;
using namespace parse;

namespace {
// Declare all matchers as globals, so they don't fall out of scope in data fn.
// Must manually type erase, since QTest can;'t figure this out on its own.
using namespace Qt::StringLiterals;
QSharedPointer<pas::ops::ConstOp<bool>> isBlank = QSharedPointer<pas::ops::generic::isBlank>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isComment = QSharedPointer<pas::ops::generic::isComment>::create();

// Instructions
QSharedPointer<pas::ops::ConstOp<bool>> isUType = QSharedPointer<pas::ops::pepp::isUType<isa::Pep10>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isRType = QSharedPointer<pas::ops::pepp::isRType<isa::Pep10>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isAType = QSharedPointer<pas::ops::pepp::isAType<isa::Pep10>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isAAAType = QSharedPointer<pas::ops::pepp::isAAAType<isa::Pep10>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isRAAAType = QSharedPointer<pas::ops::pepp::isRAAAType<isa::Pep10>>::create();
// Macro
QSharedPointer<pas::ops::ConstOp<bool>> isMacro = QSharedPointer<pas::ops::generic::isMacro>::create();
// Directives
QSharedPointer<pas::ops::ConstOp<bool>> isAlign = QSharedPointer<pas::ops::generic::isAlign>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isASCII = []() {
  auto ret = QSharedPointer<pas::ops::generic::isString>::create();
  ret->directiveAliases = {u"ASCII"_s};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isBlock = []() {
  auto ret = QSharedPointer<pas::ops::generic::isSkip>::create();
  ret->allowFill = false;
  ret->directiveAliases = {u"BLOCK"_s};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isBurn = QSharedPointer<pas::ops::pepp::isBurn>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isByte = []() {
  auto ret = QSharedPointer<pas::ops::generic::isByte1>::create();
  ret->allowMultiple = false;
  ret->directiveAliases = {u"BYTE"_s};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isEnd = QSharedPointer<pas::ops::pepp::isEnd>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isEquate = []() {
  auto ret = QSharedPointer<pas::ops::generic::isSet>::create();
  ret->directiveAliases = {u"EQUATE"_s};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isExport = QSharedPointer<pas::ops::pepp::isExport>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isImport = QSharedPointer<pas::ops::pepp::isImport>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isInput = QSharedPointer<pas::ops::pepp::isInput>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isOutput = QSharedPointer<pas::ops::pepp::isOutput>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isOrg = QSharedPointer<pas::ops::generic::isOrg>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isSCall = QSharedPointer<pas::ops::pepp::isSCall>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isSection = QSharedPointer<pas::ops::pepp::isSection>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isUSCall = QSharedPointer<pas::ops::pepp::isUSCall>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isWord = []() {
  auto ret = QSharedPointer<pas::ops::generic::isByte2>::create();
  ret->allowMultiple = false;
  ret->directiveAliases = {u"WORD"_s};
  return ret;
}();

template <typename ParserTag> struct MyHelper {
  QSharedPointer<pas::ast::Node> operator()(const std::string &input) { return nullptr; };
};

template <> struct MyHelper<pas::driver::ANTLRParserTag> {
  QSharedPointer<pas::ast::Node> operator()(const std::string &input) {
    ANTLRInputStream input_stream(input);
    PeppLexer lexer(&input_stream);
    CommonTokenStream tokens(&lexer);
    PeppLexerErrorListener listener{};
    lexer.addErrorListener(&listener);
    PeppParser parser(&tokens);
    auto *tree = parser.prog();
    REQUIRE(!listener.hadError());
    PeppASTConverter converter;
    auto ret = converter.visit(tree);
    return std::any_cast<QSharedPointer<pas::ast::Node>>(ret);
  };
};
} // namespace

using pas::ast::Node;

TEST_CASE("Passing Pepp AST conversions", "[scope:asm][kind:unit][arch:pep10]") {

  auto [name, input, fn, symbol] =
      GENERATE(table<std::string, std::string, QSharedPointer<pas::ops::ConstOp<bool>>, bool>({

          // Blank lines
          {"Blank: no spaces", "", isBlank, false},
          {"Blank: spaces", " \t", isBlank, false},

          // Comment lines
          {"Comment: no spaces", ";magic", isComment, false},
          {"Comment: spaces", " \t;magic", isComment, false},
          /*
           * Unary instructions
           */
          // U type
          {"Unary-U: no spaces", "ret", isUType, false},
          {"Unary-U: spaces", " \tret", isUType, false},
          {"Unary-U: symbol, no spaces", "s:ret", isUType, true},
          {"Unary-U: symbol, spaces", "s:\t ret", isUType, true},
          {"Unary-U: comment, no spaces", "ret;hi", isUType, false},
          {"Unary-U: comment, spaces", "ret \t;hi", isUType, false},

          // R type
          {"Unary-R: no spaces", "asla", isRType, false},
          {"Unary-R: spaces", " \taslx", isRType, false},
          {"Unary-R: symbol, no spaces", "s:rora", isRType, true},
          {"Unary-R: symbol, spaces", "s:\t rorx", isRType, true},
          {"Unary-R: comment, no spaces", "rola;rola", isRType, false},
          {"Unary-R: comment, spaces", "rolx \t;rolx", isRType, false},

          /*
           * NonUnary instructions
           */
          // A type
          {"NonUnary-A: no addr", "br x", isAType, false},
          {"NonUnary-A: addr", "br x,x", isAType, false},
          {"NonUnary-A: symbol, no spaces", "s:br x,x", isAType, true},
          {"NonUnary-A: symbol, spaces", "s: \tbr x,x", isAType, true},
          {"NonUnary-A: comment, no spaces", "br x,x;x", isAType, false},
          {"NonUnary-A: comment, spaces", "br x,x\t;x", isAType, false},

          // AAA type
          {"NonUnary-AAA: addr", "ADDSP x,i", isAAAType, false},
          {"NonUnary-AAA: symbol, no spaces", "s:ADDSP x,x", isAAAType, true},
          {"NonUnary-AAA: symbol, spaces", "s: \tSUBSP n,x", isAAAType, true},
          {"NonUnary-AAA: comment, no spaces", "scall s,x;x", isAAAType, false},
          {"NonUnary-AAA: comment, spaces", "scall x,sf\t;x", isAAAType, false},

          // RAAA type
          {"NonUnary-RAAA: addr", "adda x,i", isRAAAType, false},
          {"NonUnary-RAAA: symbol, no spaces", "s:addx x,x", isRAAAType, true},
          {"NonUnary-RAAA: symbol, spaces", "s: \tsuba n,x", isRAAAType, true},
          {"NonUnary-RAAA: comment, no spaces", "subx s,x;x", isRAAAType, false},
          {"NonUnary-RAAA: comment, spaces", "ora x,sf\t;x", isRAAAType, false},

          /*
           * Directives
           */
          // ALIGN
          {".ALIGN: mixed case", ".AlIgN 8", isAlign, false},
          {".ALIGN: symbol", "s:.align 8", isAlign, true},
          {".ALIGN: comment", ".ALIGN 8;s", isAlign, false},
          // ASCII
          {".ASCII: mixed case", ".AsCiI \"h\"", isASCII, false},
          {".ASCII: symbol", "s:.ASCII \"s\"", isASCII, true},
          {".ASCII: comment", ".ASCII \"s\";s", isASCII, false},
          {".ASCII: character", ".ascii 'a'", isASCII, false},
          {".ASCII: short string", ".ASCII \"hi\"", isASCII, false},
          {".ASCII: long string", ".ASCII \"hello\"", isASCII, false},
          // BLOCK
          {".BLOCK: mixed case", ".BlOcK 10", isBlock, false},
          {".BLOCK: symbol", "s:.BLOCK 10", isBlock, true},
          {".BLOCK: comment", ".BLOCK 10;10", isBlock, false},
          {".BLOCK: hex", ".BLOCK 0x10", isBlock, false},
          {".BLOCK: symbolic", ".BLOCK hi", isBlock, false},
          // TODO: No signed.
          // BURN
          {".BURN: mixed case", ".BuRn 0x10", isBurn, false},
          {".BURN: comment", ".BURN 0x10;10", isBurn, false},
          // BYTE
          {".BYTE: mixed case", ".ByTe 10", isByte, false},
          {".BYTE: symbol", "s:.BYTE 10", isByte, true},
          {".BYTE: comment", ".BYTE 10;10", isByte, false},
          {".BYTE: hex", ".BYTE 0x10", isByte, false},
          {".BYTE: symbolic", ".BYTE hi", isByte, false},
          {".BYTE: char", ".BYTE 'i'", isByte, false},
          {".BYTE: string", ".BYTE \"i\"", isByte, false},
          // END
          {".END: mixed case", ".EnD", isEnd, false},
          {".END: comment", ".END ;hi", isEnd, false},
          // EQUATE
          {".EQUATE: mixed case", "s:.EqUATE 10", isEquate, true},
          {".EQUATE: comment", "s:.EQUATE 10;10", isEquate, true},
          {".EQUATE: hex", "s:.EQUATE 0x10", isEquate, true},
          {".EQUATE: symbolic", "s:.EQUATE hi", isEquate, true},
          // EXPORT
          {".EXPORT: mixed case", ".ExPoRt hi", isExport, false},
          {".EXPORT: comment", ".EXPORT hi ;hi", isExport, false},
          // IMPORT
          {".IMPORT: mixed case", ".ImPoRt hi", isImport, false},
          {".IMPORT: comment", ".IMPORT hi;hi", isImport, false},
          // INPUT
          {".INPUT: mixed case", ".InPuT hi", isInput, false},
          {".INPUT: comment", ".INPUT hi;hi", isInput, false},
          // OUTPUT
          {".OUTPUT: mixed case", ".OuTpUt hi", isOutput, false},
          {".OUTPUT: comment", ".OUTPUT hi;hi", isOutput, false},
          // ORG
          {".ORG: mixed case", ".OrG 0xBAAD", isOrg, false},
          {".ORG: comment", ".ORG 0xBEEF;10", isOrg, false},
          {".ORG: symbol", "s:.ORG 0xFFAD;10", isOrg, true},
          // SCALL
          {".SCALL: mixed case", ".sCaLl hi", isSCall, false},
          {".SCALL: comment", ".SCALL hi;10", isSCall, false},
          // SECTION
          {".SECTION: mixed case", ".SeCtIoN \"data\"", isSection, false},
          {".SECTION: comment", ".SECTION \"hi\";10", isSection, false},
          // TODO: Implement
          // USCALL
          {".USCALL: mixed case", ".UsCaLl hi", isUSCall, false},
          {".USCALL: comment", ".USCALL hi;10", isUSCall, false},
          // WORD
          {".WORD: mixed case", ".WoRd 10", isWord, false},
          {".WORD: symbol", "s:.WORD 10", isWord, true},
          {".WORD: comment", ".WORD 10;10", isWord, false},
          {".WORD: hex", ".WORD 0x10", isWord, false},
          {".WORD: symbolic", ".WORD hi", isWord, false},
          {".WORD: char", ".WORD 'h'", isWord, false},
          {".WORD: string", ".WORD \"hi\"", isWord, false},
          // Macro
          {"@macro: multi-arg", "@op hi, 10", isMacro, false},
          {"@macro: symbolic", "@op hi", isMacro, false},
          {"@macro: mixed case", "@oP 10", isMacro, false},
          {"@macro: symbol", "s:@op 10", isMacro, true},
          {"@macro: comment", "@op 10;10", isMacro, false},
          {"@macro: hex", "@op 0x10", isMacro, false},
      }));
  DYNAMIC_SECTION("visitor parsing for " << name) {
    auto root = MyHelper<pas::driver::ANTLRParserTag>()(input);

    REQUIRE(root);
    auto firstChild = pas::ast::children(*root).at(0);
    // Passing tests must not generate errors
    if (firstChild->template has<pas::ast::generic::Error>()) {
      auto visit = pas::ops::generic::CollectErrors();
      pas::ast::apply_recurse<void>(*root, visit);
      for (const auto &err : visit.errors) std::cerr << err.second.message.toStdString() << std::endl;
    }
    REQUIRE(!firstChild->template has<pas::ast::generic::Error>());
    REQUIRE(symbol == firstChild->template has<pas::ast::generic::SymbolDeclaration>());
  }
  DYNAMIC_SECTION("driver parsing for " << name) {
    auto pipeline =
        pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(QString::fromStdString(input), {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    CHECK(pipelines.assemble(pas::driver::pep10::Stage::Parse));
    REQUIRE(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::IncludeMacros);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    QSharedPointer<Node> node =
        pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    REQUIRE(node.data() != nullptr);
    REQUIRE(pas::ast::children(*node).size() == 1);
    node = pas::ast::children(*node).at(0);
    bool ret = node->apply_self(*fn);
    REQUIRE(ret);
    // Passing tests must not generate errors
    REQUIRE(pas::ops::generic::collectErrors(*node).size() == 0);
    REQUIRE(symbol == node->has<pas::ast::generic::SymbolDeclaration>());
  }
}
