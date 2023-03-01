#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/errors.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/parse/pepp/node_from_parse_tree.hpp"
#include <QObject>
#include <QTest>
#include <pas/operations/generic/errors.hpp>

using SourceLocation = pas::ast::generic::SourceLocation;
using Message = pas::ast::generic::Message;
using Severity = pas::ast::generic::Message::Severity;
using Error = QPair<SourceLocation, Message>;

auto makeFatal = [](qsizetype line, QString msg) {
  return QPair<SourceLocation, Message>{
      SourceLocation{.value = {.line = line, .valid = true}},
      Message{.severity = Severity::Fatal, .message = msg}};
};

class PasOpsPepp_NodeFromParseTree_Error : public QObject {
  Q_OBJECT
private slots:
  void fail() {
    QFETCH(QString, input);
    QFETCH(QList<Error>, errors);
    auto asStd = input.toStdString();

    // Convert input string to parsed lines.
    std::vector<pas::parse::pepp::LineType> result;
    bool success = true;
    QVERIFY_THROWS_NO_EXCEPTION([&]() {
      success =
          parse(asStd.begin(), asStd.end(), pas::parse::pepp::line, result);
    }());
    QVERIFY2(success, "Failed to parse");

    auto root = pas::parse::pepp::toAST<pas::isa::Pep10ISA>(result);
    auto visit = pas::ops::generic::collectErrors();
    pas::ast::apply_recurse<void>(*root, visit);
    auto actualErrors = visit.errors;
    for (int it = 0; it < qMin(errors.size(), actualErrors.size()); it++) {
      QCOMPARE(errors[it].first, actualErrors[it].first);
      QCOMPARE(errors[it].second.message, actualErrors[it].second.message);
      QCOMPARE(errors[it].second.severity, actualErrors[it].second.severity);
    }
    QCOMPARE(errors.size(), actualErrors.size());
  }
  void fail_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QList<Error>>("errors");
    namespace E = pas::errors::pepp;

    // Unary Instructions
    QTest::addRow("unary: invalid mnemonic")
        << u"rat"_qs << QList<Error>{makeFatal(0, E::invalidMnemonic)};

    // NonUnary Instructions -- BR/Call family
    QTest::addRow("nonunary: invalid mnemonic")
        << u"brg k"_qs << QList<Error>{makeFatal(0, E::invalidMnemonic)};
    QTest::addRow("nonunary: 3-byte string")
        << u"br \"abc\""_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    // Check that 0xFFFF->0x1_0000 triggers hex constant to be too big.
    QTest::addRow("nonunary: 2-byte hex") << u"br 0xFFFF"_qs << QList<Error>{};
    QTest::addRow("nonunary: 3-byte hex")
        << u"br 0x10000"_qs << QList<Error>{makeFatal(0, E::hexTooBig2)};
    QTest::addRow("nonunary: illegal addressing mode")
        << u"br 0x10,sf"_qs << QList<Error>{makeFatal(0, E::illegalAddrMode)};

    // NonUnary Instructions -- Not stores
    // - requires address mode
    QTest::addRow("nonunary: missing address mode-1 byte")
        << u"ldba 0xd"_qs << QList<Error>{makeFatal(0, E::requiredAddrMode)};
    QTest::addRow("nonunary: missing address mode-2 byte")
        << u"ldwa 0xda"_qs << QList<Error>{makeFatal(0, E::requiredAddrMode)};
    // - max 2 byte arguments
    QTest::addRow("nonunary: max 1 byte arguments")
        << u"ldba 0x0bada,d"_qs << QList<Error>{makeFatal(0, E::hexTooBig1)};
    QTest::addRow("nonunary: max 2 byte arguments")
        << u"ldwa 0xabadbeefee,d"_qs
        << QList<Error>{makeFatal(0, E::hexTooBig2)};

    // NonUnary Instructions -- Stores
    // - don't allow I addressing mode-Immediate addressing seems to be allowed
    QTest::addRow("nonunary store: I is bad addressing mode-1 byte")
        << u"stba 0xfc16,i"_qs
        << QList<Error>{makeFatal(0, E::invalidAddrMode)};
    QTest::addRow("nonunary store: I is bad addressing mode-2 byte")
        << u"stwa 0xfc16,i"_qs
        << QList<Error>{makeFatal(0, E::invalidAddrMode)};

    // max 2 byte arguments
    /*
     * Directives
     */
    //  Message that return variables need to be converted to string for compare
    //  to work.
    const QString arg0 = E::expectNArguments.arg(0);
    const QString arg1 = E::expectNArguments.arg(1);
    const QString ascii = E::dotRequiresString.arg(".ASCII");
    const QString end = E::noDefineSymbol.arg(".END");

    //    const QString missing( "Message not in errors.hpp");

    // ALIGN
    // - only decimal powers of 2 args
    QTest::addRow(".ALIGN: power of 2")
        << u".ALIGN 3"_qs << QList<Error>{makeFatal(0, E::alignPow2)};
    // - exactly 1 arg
    QTest::addRow(".ALIGN: max 1 argument")
        << u".ALIGN 2,4"_qs << QList<Error>{makeFatal(0, arg1)};

    // ASCII
    // - no chars
    QTest::addRow(".ASCII: no characters")
        << u".ASCII"_qs << QList<Error>{makeFatal(0, arg1)};
    // - no numeric (unsigned/signed decimal / hex)
    //  ".ASCII requires a string constant argument." - message not in
    //  errors.hpp
    QTest::addRow(".ASCII: no hex")
        << u".ASCII 0xdad"_qs << QList<Error>{makeFatal(0, ascii)};
    //  ".ASCII requires a string constant argument." - message not in
    //  errors.hpp
    QTest::addRow(".ASCII: no unsigned decimals")
        << u".ASCII 42"_qs << QList<Error>{makeFatal(0, ascii)};
    //  ".ASCII requires a string constant argument." - message not in
    //  errors.hpp
    QTest::addRow(".ASCII: no signed decimals")
        << u".ASCII -42"_qs << QList<Error>{makeFatal(0, ascii)};
    // - exactly 1 arg
    QTest::addRow(".ASCII: max 1 argument")
        << u".ASCII \"Bad\" \"Beef\""_qs << QList<Error>{makeFatal(0, arg1)};

    // BLOCK
    // - no signed
    QTest::addRow(".BLOCK: no negative decimals")
        << u".BLOCK -42"_qs << QList<Error>{makeFatal(0, E::decTooBig2)};
    // - no chars
    QTest::addRow(".BLOCK: no characters")
        << u".BLOCK '*'"_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    // - no strings (long or short)
    QTest::addRow(".BLOCK: no strings")
        << u".BLOCK \"Bad\""_qs
        << QList<Error>{makeFatal(0, E::expectedNumeric)};
    // - exactly 1 arg
    QTest::addRow(".BLOCK: max 1 argument")
        << u".BLOCK 12,34"_qs << QList<Error>{makeFatal(0, arg1)};
    // - arg must fit in 16 bits (no big strs / hex / signed / unsigned)
    QTest::addRow(".BLOCK: must fit in 16 bits")
        << u".BLOCK 0xbadbeef"_qs << QList<Error>{makeFatal(0, E::hexTooBig2)};
    // - no chars

    // BURN -- no symbol, only allows hex
    // - no decimal
    QTest::addRow(".BURN: no unsigned decimals")
        << u".BURN 33333"_qs << QList<Error>{makeFatal(0, E::burnRequiresHex)};
    QTest::addRow(".BURN: no negative decimals")
        << u".BURN -42"_qs << QList<Error>{makeFatal(0, E::burnRequiresHex)};
    // - no text (chars/longstr/shortstr)
    QTest::addRow(".BURN: no characters")
        << u".BLOCK '*'"_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    QTest::addRow(".BURN: no strings")
        << u".BLOCK \"Bad\""_qs
        << QList<Error>{makeFatal(0, E::expectedNumeric)};
    // - no symbol
    QTest::addRow(".BURN: no symbols")
        << u"ret .BURN"_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    // - exactly 1 arg
    QTest::addRow(".BURN: exactly 1 argument")
        << u".BLOCK \"Very\", \"Bad\""_qs << QList<Error>{makeFatal(0, arg1)};
    // - arg must fit in 16 bits
    QTest::addRow(".BURN: fit in 16 bits-hex")
        << u".BLOCK 0xbadbeef"_qs << QList<Error>{makeFatal(0, E::hexTooBig2)};
    QTest::addRow(".BURN: fit in 16 bits-decimal")
        << u".BLOCK 666666"_qs << QList<Error>{makeFatal(0, E::decTooBig2)};
    QTest::addRow(".BURN: fit in 16 bits-negative decimal")
        << u".BLOCK -32679"_qs << QList<Error>{makeFatal(0, E::decTooBig2)};

    // Byte
    // - exactly 1 arg
    QTest::addRow(".BYTE: min 1 argument")
        << u".BYTE"_qs << QList<Error>{makeFatal(0, arg1)};
    QTest::addRow(".BYTE: max 1 argument")
        << u".BYTE 0x00, 0x01"_qs << QList<Error>{makeFatal(0, arg1)};
    // - arg must fit in 8 bits
    QTest::addRow(".BYTE: no characters")
        << u".BYTE '*'"_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    QTest::addRow(".BYTE: no strings")
        << u".BYTE \"Bad\""_qs
        << QList<Error>{makeFatal(0, E::expectedNumeric)};
    QTest::addRow(".BYTE: fit in 8 bits-hex")
        << u".BYTE 0x0bad"_qs << QList<Error>{makeFatal(0, E::hexTooBig1)};
    QTest::addRow(".BYTE: fit in 8 bits-decimal")
        << u".BYTE 256"_qs << QList<Error>{makeFatal(0, E::decTooBig1)};
    QTest::addRow(".BYTE: fit in 8 bits-negative decimal")
        << u".BYTE -129"_qs << QList<Error>{makeFatal(0, E::decTooBig1)};

    // End
    // - no symbol
    QTest::addRow(".END: no symbol")
        << u"ret: .END"_qs << QList<Error>{makeFatal(0, end)};
    // - exactly 0 args
    QTest::addRow(".END: exactly 0 arguments")
        << u".END 1"_qs << QList<Error>{makeFatal(0, arg0)};

    // Equate
    // - requires symbol
    QTest::addRow(".EQUATE: no symbol")
        << u".EQUATE 10"_qs
        << QList<Error>{makeFatal(0, E::equateRequiresSymbol)};
    // - exactly 1 argument
    QTest::addRow(".EQUATE: max 1 arguement")
        << u"failure: .EQUATE 10,0x1234"_qs << QList<Error>{makeFatal(0, arg1)};
    // - arg fits in 16 bits
    QTest::addRow(".EQUATE: fit in 16 bits-decimal")
        << u"failure: .EQUATE 666666"_qs
        << QList<Error>{makeFatal(0, E::decTooBig2)};
    QTest::addRow(".EQUATE: fit in 16 bits-hex")
        << u"failure: .EQUATE 0xbadbeef"_qs
        << QList<Error>{makeFatal(0, E::hexTooBig2)};

    // Export / Import / Input / Output / SCall / USCall
    QString sharedSymbols[]{"EXPORT", "IMPORT", "INPUT",
                            "OUTPUT", "SCALL",  "USCALL"};

    //  Loop through symbols above
    for (auto &symbol : sharedSymbols) {
      const bool isCall = symbol.contains("CALL");
      // - no symbol
      QString label = symbol + u": no symbols"_qs;
      QString command = QString("ret: %1").arg(symbol);
      QTest::addRow(label.toUtf8())
          << command << QList<Error>{makeFatal(0, E::invalidMnemonic)};
      // - exactly 1 arg
      label = symbol + u": min 1 argument"_qs;
      command = QString("%1").arg(symbol);
      QTest::addRow(label.toUtf8())
          << command << QList<Error>{makeFatal(0, E::invalidMnemonic)};
      label = symbol + u": max 1 argument"_qs;
      command = QString("%1 0x00, 0x01").arg(symbol);
      QTest::addRow(label.toUtf8())
          << command
          << QList<Error>{makeFatal(0, isCall ? E::requiredAddrMode
                                              : E::invalidMnemonic)};
      // - arg must be symbolic
      label = symbol + u": arg must be symbolic"_qs;
      command = QString("%1 \"bad\"").arg(symbol);
      QTest::addRow(label.toUtf8())
          << command
          << QList<Error>{makeFatal(0, isCall ? E::expectedNumeric
                                              : E::invalidMnemonic)};
      // - arg must not be non-symbolic
      label = symbol + u": arg must not be non-symbolic"_qs;
      command = QString("%1 0xbad").arg(symbol);
      QTest::addRow(label.toUtf8())
          << command
          << QList<Error>{makeFatal(0, isCall ? E::requiredAddrMode
                                              : E::invalidMnemonic)};
    }

    // Section
    // - exactly 1 arg (an identifier)
    QTest::addRow("SECTION: min 1 argument")
        << u"SECTION"_qs << QList<Error>{makeFatal(0, E::invalidMnemonic)};
    QTest::addRow("SECTION: max 1 argument")
        << u"SECTION 0x00, 0x01"_qs
        << QList<Error>{makeFatal(0, E::invalidMnemonic)};
    // - no symbol
    QTest::addRow("SECTION: no symbol")
        << u"ret: SECTION 1"_qs
        << QList<Error>{makeFatal(0, E::invalidMnemonic)};

    // Word
    // - exactly 1 arg
    QTest::addRow(".WORD: min 1 argument")
        << u".WORD"_qs << QList<Error>{makeFatal(0, arg1)};
    QTest::addRow(".WORD: max 1 argument")
        << u".WORD 0x0bad, 0x0dad"_qs << QList<Error>{makeFatal(0, arg1)};
    // - arg must fit in 16 bits
    QTest::addRow(".WORD: no characters")
        << u".WORD '*'"_qs << QList<Error>{makeFatal(0, E::expectedNumeric)};
    QTest::addRow(".WORD: no strings")
        << u".WORD \"Bad\""_qs
        << QList<Error>{makeFatal(0, E::expectedNumeric)};
    QTest::addRow(".WORD: fit in 16 bits-hex")
        << u".WORD 0x0baadbeef"_qs << QList<Error>{makeFatal(0, E::hexTooBig2)};
    QTest::addRow(".WORD: fit in 16 bits-decimal")
        << u".WORD 65536"_qs << QList<Error>{makeFatal(0, E::decTooBig2)};
    QTest::addRow(".WORD: fit in 16 bits-negative decimal")
        << u".WORD -32679"_qs << QList<Error>{makeFatal(0, E::decTooBig2)};

    //  Missing errors
    //  invalidDirective
    //  invalidSection
    //  argAfterMnemonic
    //  expectedSymbolic
    //  strTooLong1
    //  strTooLong2
  }
};

#include "error.test.moc"

QTEST_MAIN(PasOpsPepp_NodeFromParseTree_Error)
