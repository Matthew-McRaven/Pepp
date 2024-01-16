/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "isa/pep10.hpp"
#include "asm/pas/ast/generic/attr_error.hpp"
#include "asm/pas/ast/generic/attr_location.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/errors.hpp"
#include "asm/pas/operations/generic/errors.hpp"

#undef emit
#include "asm/parse/PeppLexer.h"
#include "asm/parse/PeppParser.h"
#include "asm/parse/PeppLexerErrorListener.h"
#include "asm/pas/parse/pepp/PeppASTConverter.h"

using namespace antlr4;
using namespace parse;

using SourceLocation = pas::ast::generic::SourceLocation;
using Message = pas::ast::generic::Message;
using Severity = pas::ast::generic::Message::Severity;
using Error = QPair<SourceLocation, Message>;

auto makeFatal = [](qsizetype line, QString msg) {
  return QPair<SourceLocation, Message>{
      SourceLocation{.value = {.line = line, .valid = true}},
      Message{.severity = Severity::Fatal, .message = msg}};
};
namespace E = pas::errors::pepp;
const QString arg0 = E::expectNArguments.arg(0);
const QString arg1 = E::expectNArguments.arg(1);
const QString ascii = E::dotRequiresString.arg(".ASCII");
const QString _end = E::noDefineSymbol.arg(".END");

template <typename ParserTag>
struct MyHelper {
    QSharedPointer<pas::ast::Node> operator()(const QString& asQString){return nullptr;};
    static std::string name() {return "";};
};

template <>
struct MyHelper<pas::driver::ANTLRParserTag> {
    QSharedPointer<pas::ast::Node> operator()(const QString& asQString){
        auto input = asQString.toStdString();
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
    static std::string name() {return "ANTLR4";};
};

TEST_CASE("Pepp AST conversion, failing", "[parse]") {
    //  Message that return variables need to be converted to string for compare
    //  to work.



    auto [name, input, errors] = GENERATE(table<std::string, QString, QList<Error>>({

        // Unary Instructions
        {"unary: invalid mnemonic",u"rat"_qs, QList<Error>{makeFatal(0, E::invalidMnemonic)}},

        // NonUnary Instructions -- BR/Call family
        {"nonunary: invalid mnemonic",u"brg k"_qs, QList<Error>{makeFatal(0, E::invalidMnemonic)}},
        {"nonunary: 3-byte string",u"br \"abc\""_qs, QList<Error>{makeFatal(0, E::expectedNumeric)}},
        // Check that 0xFFFF->0x1_0000 triggers hex constant to be too big.
        // {"nonunary: 2-byte hex",u"br 0xFFFF"_qs <<
        // QList<Error>{}},
        {"nonunary: 3-byte hex",u"br 0x10000"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},
        {"nonunary: illegal addressing mode",u"br 0x10,sf"_qs, QList<Error>{makeFatal(0, E::illegalAddrMode)}},

        // NonUnary Instructions -- Not stores
        // - requires address mode
        {"nonunary: missing address mode-1 byte",u"ldba 0xd"_qs, QList<Error>{makeFatal(0, E::requiredAddrMode)}},
        {"nonunary: missing address mode-2 byte",u"ldwa 0xda"_qs, QList<Error>{makeFatal(0, E::requiredAddrMode)}},
        // - max 2 byte arguments
        // LDBA must allow 2 byte operands. Non-I addressing modes need 2 bytes to
        // access all of main memory. Undecided (2023-03-01if LDBA i should allow
        // 2 bytes.
        {"nonunary: max 2 byte arguments",u"ldba 0xabadbeefee,d"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},
        {"nonunary: max 2 byte arguments",u"ldwa 0xabadbeefee,d"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},

        // NonUnary Instructions -- Stores
        // - don't allow I addressing mode
        {"nonunary store: I is bad addressing mode-1 byte",u"stba 0xfc16,i"_qs, QList<Error>{makeFatal(0, E::illegalAddrMode)}},
        {"nonunary store: I is bad addressing mode-2 byte",u"stwa 0xfc16,i"_qs, QList<Error>{makeFatal(0, E::illegalAddrMode)}},

        // ALIGN
        // - only decimal powers of 2 args
        {".ALIGN: power of 2",u".ALIGN 3"_qs, QList<Error>{makeFatal(0, E::alignPow2)}},
        // - exactly 1 arg
        {".ALIGN: max 1 argument",u".ALIGN 2,4"_qs, QList<Error>{makeFatal(0, arg1)}},

        // ASCII
        // - no chars
        {".ASCII: no characters",u".ASCII"_qs, QList<Error>{makeFatal(0, arg1)}},
        // - no numeric (unsigned/signed decimal / hex)
        //  ".ASCII requires a string constant argument." - message not in
        //  errors.hpp
        {".ASCII: no hex",u".ASCII 0xdad"_qs, QList<Error>{makeFatal(0, ascii)}},
        //  ".ASCII requires a string constant argument." - message not in
        //  errors.hpp
        {".ASCII: no unsigned decimals",u".ASCII 42"_qs, QList<Error>{makeFatal(0, ascii)}},
        //  ".ASCII requires a string constant argument." - message not in
        //  errors.hpp
        {".ASCII: no signed decimals",u".ASCII -42"_qs, QList<Error>{makeFatal(0, ascii)}},
        // - exactly 1 arg
        {".ASCII: max 1 argument",u".ASCII \"Bad\", \"Beef\""_qs, QList<Error>{makeFatal(0, arg1)}},

        // BLOCK
        // - no signed
        {".BLOCK: no negative decimals",u".BLOCK -42"_qs, QList<Error>{makeFatal(0, E::decUnsigned2)}},
        // - no chars
        {".BLOCK: no characters",u".BLOCK '*'"_qs, QList<Error>{makeFatal(0, E::expectedNumeric)}},
        // - no strings (long or short)
        {".BLOCK: no strings",u".BLOCK \"Bad\""_qs, QList<Error>{makeFatal(0, E::expectedNumeric)}},
        // - exactly 1 arg
        {".BLOCK: max 1 argument",u".BLOCK 12,34"_qs, QList<Error>{makeFatal(0, arg1)}},
        // - arg must fit in 16 bits (no big strs / hex / signed / unsigned)
        {".BLOCK: must fit in 16 bits",u".BLOCK 0xbadbeef"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},
        // - no chars

        // BURN -- no symbol, only allows hex
        // - no decimal
        {".BURN: no unsigned decimals",u".BURN 33333"_qs, QList<Error>{makeFatal(0, E::requiresHex.arg(".BURN"))}},
        {".BURN: no negative decimals",u".BURN -42"_qs, QList<Error>{makeFatal(0, E::requiresHex.arg(".BURN"))}},
        // - no text (chars/longstr/shortstr)
        {".BURN: no characters",u".BLOCK '*'"_qs, QList<Error>{makeFatal(0, E::expectedNumeric)}},
        {".BURN: no strings",u".BLOCK \"Bad\""_qs, QList<Error>{makeFatal(0, E::expectedNumeric)}},
        // - no symbol
        {".BURN: no symbols",u"ret: .BURN 0xfe"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg((".BURN")))}},
        // - exactly 1 arg
        {".BURN: exactly 1 argument",u".BLOCK \"Very\", \"Bad\""_qs, QList<Error>{makeFatal(0, arg1)}},
        // - arg must fit in 16 bits
        {".BURN: fit in 16 bits-hex",u".BLOCK 0xbadbeef"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},

        // Byte
        // - exactly 1 arg
        {".BYTE: min 1 argument",u".BYTE"_qs, QList<Error>{makeFatal(0, arg1)}},
        {".BYTE: max 1 argument",u".BYTE 0x00, 0x01"_qs, QList<Error>{makeFatal(0, arg1)}},
        // - arg must fit in 8 bits

        {".BYTE: no long strings",u".BYTE \"Bad\""_qs, QList<Error>{makeFatal(0, E::strTooLong1)}},
        {".BYTE: fit in 8 bits-hex",u".BYTE 0x0bad"_qs, QList<Error>{makeFatal(0, E::hexTooBig1)}},
        {".BYTE: fit in 8 bits-decimal",u".BYTE 256"_qs, QList<Error>{makeFatal(0, E::decTooBig1)}},
        {".BYTE: fit in 8 bits-negative decimal",u".BYTE -129"_qs, QList<Error>{makeFatal(0, E::decTooBig1)}},

        // End
        // - no symbol
        {".END: no symbol",u"ret: .END"_qs, QList<Error>{makeFatal(0, _end)}},
        // - exactly 0 args
        {".END: exactly 0 arguments",u".END 1"_qs, QList<Error>{makeFatal(0, arg0)}},

        // Equate
        // - requires symbol
        {".EQUATE: no symbol",u".EQUATE 10"_qs, QList<Error>{makeFatal(0, E::equateRequiresSymbol)}},
        // - exactly 1 argument
        {".EQUATE: max 1 arguement",u"failure: .EQUATE 10,0x1234"_qs, QList<Error>{makeFatal(0, arg1)}},
        // - arg fits in 16 bits
        {".EQUATE: fit in 16 bits-decimal",u"failure: .EQUATE 666666"_qs, QList<Error>{makeFatal(0, E::decTooBig2)}},
        {".EQUATE: fit in 16 bits-hex",u"failure: .EQUATE 0xbadbeef"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},

        {".EXPORT: no symbols",u"x: .EXPORT y"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".EXPORT"))}},
        {".EXPORT: min 1 arg",u".EXPORT"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".EXPORT: max 1 arg",u".EXPORT hi, world"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".EXPORT: require symbolic arg",u".EXPORT 10"_qs, QList<Error>{makeFatal(0, E::expectedSymbolic)}},
        {".IMPORT: no symbols",u"x: .IMPORT y"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".IMPORT"))}},
        {".IMPORT: min 1 arg",u".IMPORT"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".IMPORT: max 1 arg",u".IMPORT hi, world"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".IMPORT: require symbolic arg",u".IMPORT 10"_qs, QList<Error>{makeFatal(0, E::expectedSymbolic)}},
        {".INPUT: no symbols",u"x: .INPUT y"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".INPUT"))}},
        {".INPUT: min 1 arg",u".INPUT"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".INPUT: max 1 arg",u".INPUT hi, world"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".INPUT: require symbolic arg",u".INPUT 10"_qs, QList<Error>{makeFatal(0, E::expectedSymbolic)}},
        {".OUTPUT: no symbols",u"x: .OUTPUT y"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".OUTPUT"))}},
        {".OUTPUT: min 1 arg",u".OUTPUT"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".OUTPUT: max 1 arg",u".OUTPUT hi, world"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".OUTPUT: require symbolic arg",u".OUTPUT 10"_qs, QList<Error>{makeFatal(0, E::expectedSymbolic)}},
        {".SCALL: no symbols",u"x: .SCALL y"_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".SCALL"))}},
        {".SCALL: min 1 arg",u".SCALL"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".SCALL: max 1 arg",u".SCALL hi, world"_qs, QList<Error>{makeFatal(0, E::expectNArguments.arg(1))}},
        {".SCALL: require symbolic arg",u".SCALL 10"_qs, QList<Error>{makeFatal(0, E::expectedSymbolic)}},

        // ORG
        {".ORG: no unsigned decimals",u".ORG 33333"_qs, QList<Error>{makeFatal(0, E::requiresHex.arg(".ORG"))}},
        {".ORG: no negative decimals",u".ORG -42"_qs, QList<Error>{makeFatal(0, E::requiresHex.arg(".ORG"))}},

        // Section
        // - exactly 1 arg (an identifier)
        {".SECTION: min 1 argument",u".SECTION"_qs, QList<Error>{makeFatal(0, E::expectNMArguments.arg(1, 2))}},
        {".SECTION: max 2 argument",u".SECTION 0x0, 0x1, 0x2"_qs, QList<Error>{makeFatal(0, E::expectNMArguments.arg(1, 2))}},
        // - no symbol
        {".SECTION: no symbol",u"ret: .SECTION \"data\""_qs, QList<Error>{makeFatal(0, E::noDefineSymbol.arg(".SECTION"))}},

        // Word
        // - exactly 1 arg
        {".WORD: min 1 argument",u".WORD"_qs, QList<Error>{makeFatal(0, arg1)}},
        {".WORD: max 1 argument",u".WORD 0x0bad, 0x0dad"_qs, QList<Error>{makeFatal(0, arg1)}},
        // - arg must fit in 16 bits
        {".WORD: no long strings",u".WORD \"Bad\""_qs, QList<Error>{makeFatal(0, E::strTooLong2)}},
        {".WORD: fit in 16 bits-hex",u".WORD 0x0baadbeef"_qs, QList<Error>{makeFatal(0, E::hexTooBig2)}},
        {".WORD: fit in 16 bits-decimal",u".WORD 65536"_qs, QList<Error>{makeFatal(0, E::decTooBig2)}},
        {".WORD: fit in 16 bits-negative decimal",u".WORD -32769"_qs, QList<Error>{makeFatal(0, E::decTooBig2)}},

        //  Missing errors
        //  invalidDirective
        //  invalidSection
        //  argAfterMnemonic
        //  strTooLong1
        //  strTooLong2

    }));
    DYNAMIC_SECTION("visitor parsing for " << name) {
        auto root = MyHelper<pas::driver::ANTLRParserTag>()(input);
        auto visit = pas::ops::generic::CollectErrors();
        pas::ast::apply_recurse<void>(*root, visit);
        auto actualErrors = visit.errors;
        for (int it = 0; it < qMin(errors.size(), actualErrors.size()); it++) {
            REQUIRE(errors[it].first == actualErrors[it].first);
            if (errors[it].second.message != actualErrors[it].second.message) {
                std::cerr << errors[it].second.message.toStdString() << " != " << actualErrors[it].second.message.toStdString() << std::endl;
            }
            REQUIRE(errors[it].second.message == actualErrors[it].second.message);
            REQUIRE(errors[it].second.severity == actualErrors[it].second.severity);
        }
        REQUIRE(errors.size() == actualErrors.size());
    }

    DYNAMIC_SECTION("driver parsing for " << name) {
        auto asStd = input.toStdString();

        auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
        auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
        pipelines.pipelines.push_back(pipeline);
        REQUIRE(!pipelines.assemble(pas::driver::pep10::Stage::Parse));
        REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));

        QSharedPointer<pas::ast::Node> node =
            pipelines.pipelines[0]
                .first->bodies[pas::driver::repr::Nodes::name]
                .value<pas::driver::repr::Nodes>()
                .value;
        REQUIRE(node.data() != nullptr);
        auto visit = pas::ops::generic::CollectErrors();

        pas::ast::apply_recurse<void>(*node, visit);
        auto actualErrors = visit.errors;
        for (int it = 0; it < qMin(errors.size(), actualErrors.size()); it++) {
            REQUIRE(errors[it].first == actualErrors[it].first);
            REQUIRE(errors[it].second.message == actualErrors[it].second.message);
            REQUIRE(errors[it].second.severity == actualErrors[it].second.severity);
        }
        REQUIRE(errors.size() == actualErrors.size());
    }
}

int main(int argc, char* argv[]) {
    return Catch::Session().run( argc, argv );
}
