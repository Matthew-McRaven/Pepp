#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/pepp/assign_addr.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

class PasOpsPepp_FormatListing : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, source);
    QFETCH(QStringList, listing);
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    auto str = parsed.errors.join("\n").toStdString();
    QVERIFY2(!parsed.hadError, str.data());
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    auto actualListing = pas::ops::pepp::formatListing<pas::isa::Pep10ISA>(
        *parsed.root, {.bytesPerLine = 3});
    // FIXME: remove
    auto actualListingText = actualListing.join("\n").toStdString();
    QVERIFY2(actualListing.size() == listing.size(), actualListingText.data());
    QCOMPARE(actualListing, listing);
  }

  void smoke_data() {
    QTest::addColumn<QString>("source");
    QTest::addColumn<QStringList>("listing");

    QTest::addRow("Blank") << "\n" << QStringList{"", ""};
    QTest::addRow("Comment")
        << ";hello\n;world"
        << QStringList{"             ;hello", "             ;world"};

    QTest::addRow("Unary") << "asla\nasra"
                           << QStringList{"0000     14         ASLA",
                                          "0001     16         ASRA"};
    QTest::addRow("Unary + symbol")
        << "abcdefg:asla" << QStringList{"0000     14 abcdefg: ASLA"};
    QTest::addRow("Nonunary non-br")
        << "adda 0xfaad,i\nsuba 0xbaad,sfx"
        << QStringList{"0000 A0FAAD         ADDA    0xFAAD, i",
                       "0003 B7BAAD         SUBA    0xBAAD, sfx"};
    QTest::addRow("Nonunary br")
        << "br 10,i\nbr 20,x"
        << QStringList{"0000 1C000A         BR      10",
                       "0003 1D0014         BR      20, x"};

    QTest::addRow("ALIGN 1")
        << ".ALIGN 1" << QStringList{"0000                .ALIGN  1"};
    QTest::addRow("ALIGN 2 @ 0")
        << ".ALIGN 2" << QStringList{"0000                .ALIGN  2"};
    QTest::addRow("ALIGN 2 @ 1")
        << ".BYTE 1\n.ALIGN 2"
        << QStringList{"0000     01         .BYTE   1",
                       "0001     00         .ALIGN  2"};
    QTest::addRow("ALIGN 4 @ 0")
        << ".ALIGN 4" << QStringList{"0000                .ALIGN  4"};
    QTest::addRow("ALIGN 4 @ 1")
        << ".BYTE 1\n.ALIGN 4"
        << QStringList{"0000     01         .BYTE   1",
                       "0001 000000         .ALIGN  4"};
    QTest::addRow("ALIGN 8 @ 0")
        << ".ALIGN 8" << QStringList{"0000                .ALIGN  8"};
    QTest::addRow("ALIGN 8 @ 1") << ".BYTE 1\n.ALIGN 8"
                                 << QStringList{"0000     01         .BYTE   1",
                                                "0001 000000         .ALIGN  8",
                                                "     000000", "         00"};
    QTest::addRow("ALIGN 8 @ 2")
        << ".WORD 1\n.ALIGN 8"
        << QStringList{"0000   0001         .WORD   1",
                       "0002 000000         .ALIGN  8", "     000000"};

    QTest::addRow("ASCII 2-string")
        << ".ASCII \"hi\"" << QStringList{"0000   6869         .ASCII  \"hi\""};
    QTest::addRow("ASCII 3-string")
        << ".ASCII \"hel\""
        << QStringList{"0000 68656C         .ASCII  \"hel\""};
    QTest::addRow("ASCII >3-string")
        << ".ASCII \"hello\""
        << QStringList{"0000 68656C         .ASCII  \"hello\"", "       6C6F"};

    QTest::addRow("BLOCK 0")
        << "s: .BLOCK 0" << QStringList{"0000        s:       .BLOCK  0"};
    QTest::addRow("BLOCK 1")
        << ".BLOCK 1" << QStringList{"0000     00         .BLOCK  1"};
    QTest::addRow("BLOCK 0x2")
        << ".BLOCK 0x2" << QStringList{"0000   0000         .BLOCK  0x0002"};
    QTest::addRow("BLOCK 4")
        << ".BLOCK 4"
        << QStringList{"0000 000000         .BLOCK  4", "         00"};

    QTest::addRow("BYTE 0xFE")
        << ".BYTE 0xFE" << QStringList{"0000     FE         .BYTE   0xFE"};

    QTest::addRow("END: no comment")
        << ".END" << QStringList{"                    .END"};
    QTest::addRow("END: comment")
        << ".END;the world"
        << QStringList{"                    .END            ;the world"};

    QTest::addRow("EQUATE: no comment")
        << "s:.EQUATE 10" << QStringList{"            s:       .EQUATE 10"};
    QTest::addRow("EQUATE: comment")
        << "s:.EQUATE 10;hi"
        << QStringList{"            s:       .EQUATE 10      ;hi"};
    QTest::addRow("EQUATE: symbolic")
        << "s:.EQUATE y\n.block 0x3\ny:.block 0"
        << QStringList{"            s:       .EQUATE y",
                       "0000 000000         .BLOCK  0x0003",
                       "0003        y:       .BLOCK  0"};
    QTest::addRow("EQUATE: hex")
        << "s:.EQUATE 0x2"
        << QStringList{"            s:       .EQUATE 0x0002"};
    QTest::addRow("EQUATE: unsigned")
        << "s:.EQUATE 2" << QStringList{"            s:       .EQUATE 2"};
    QTest::addRow("EQUATE: signed")
        << "s: .EQUATE -2" << QStringList{"            s:       .EQUATE -2"};
    QTest::addRow("EQUATE: char")
        << "s: .EQUATE 's'" << QStringList{"            s:       .EQUATE 's'"};
    QTest::addRow("EQUATE: string")
        << "s: .EQUATE \"hi\""
        << QStringList{"            s:       .EQUATE \"hi\""};
    QTest::addRow("ORG") << ".BLOCK 1\n.ORG 0x8000\n.BLOCK 1"
                         << QStringList{"0000     00         .BLOCK  1",
                                        "                    .ORG    0x8000",
                                        "8000     00         .BLOCK  1"};
    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
      QTest::addRow("%s", str)
          << u"%1 s"_qs.arg(str)
          << QStringList{u"                    %1s"_qs.arg(str, -8)};
    }

    QTest::addRow("WORD 0xFFFE")
        << ".WORD 0xFFFE" << QStringList{"0000   FFFE         .WORD   0xFFFE"};
  }

  void macros() {}
  void macro_data() {}
};

#include "listing.test.moc"

QTEST_MAIN(PasOpsPepp_FormatListing)
