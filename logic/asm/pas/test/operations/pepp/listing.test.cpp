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
    auto actualListing =
        pas::ops::pepp::formatListing<pas::isa::Pep10ISA>(*parsed.root, 3);
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
        << QStringList{"0000 A0FAAD         ADDA   0xFAAD, i",
                       "0003 B7BAAD         SUBA   0xBAAD, sfx"};
    QTest::addRow("Nonunary br")
        << "br 10,i\nbr 20,x"
        << QStringList{"0000 1C000A         BR     10",
                       "0003 1D0014         BR     20, x"};

    QTest::addRow("ALIGN 1")
        << ".ALIGN 1" << QStringList{"0000                .ALIGN 1"};
    QTest::addRow("ALIGN 2 @ 0")
        << ".ALIGN 2" << QStringList{"0000                .ALIGN 2"};
    QTest::addRow("ALIGN 2 @ 1") << ".BYTE 1\n.ALIGN 2"
                                 << QStringList{"0000     01         .BYTE  1",
                                                "0001     00         .ALIGN 2"};
    QTest::addRow("ALIGN 4 @ 0")
        << ".ALIGN 4" << QStringList{"0000                .ALIGN 4"};
    QTest::addRow("ALIGN 4 @ 1") << ".BYTE 1\n.ALIGN 4"
                                 << QStringList{"0000     01         .BYTE  1",
                                                "0001 000000         .ALIGN 4"};
    // QTest::addRow("ALIGN 8 @ 0") << "\n" << QStringList{""};
    // QTest::addRow("ALIGN 8 @ 1") << "\n" << QStringList{""};
    // QTest::addRow("ALIGN 8 @ 2") << "\n" << QStringList{""};

    // QTest::addRow("ASCII 2-string") << "\n" << QStringList{""};
    // QTest::addRow("ASCII 3-string") << "\n" << QStringList{""};
    // QTest::addRow("ASCII 4-string") << "\n" << QStringList{""};

    // QTest::addRow("BLOCK 0") << "\n" << QStringList{""};
    // QTest::addRow("BLOCK 1") << "\n" << QStringList{""};
    // QTest::addRow("BLOCK 2") << "\n" << QStringList{""};
    // QTest::addRow("BLOCK 4") << "\n" << QStringList{""};

    // QTest::addRow("BYTE 0xFE") << "\n" << QStringList{""};

    // QTest::addRow("END: no comment") << "\n" << QStringList{""};
    // QTest::addRow("END: comment") << "\n" << QStringList{""};

    // QTest::addRow("EQUATE: no comment") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: comment") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: symbolic") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: hex") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: unsigned") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: signed") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: char") << "\n" << QStringList{""};
    // QTest::addRow("EQUATE: string") << "\n" << QStringList{""};

    /*for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
        QTest::addRow(str) << QString::fromStdString(str) << QStringList{};
    }*/

    // QTest::addRow("WORD 0xFFFE") << "\n" << QStringList{""};*/
  }

  void macros() {}
  void macro_data() {}
};

#include "listing.test.moc"

QTEST_MAIN(PasOpsPepp_FormatListing)
