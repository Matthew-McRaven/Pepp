#include <QTest>
#include <QObject>

#include "pas/isa/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/operations/pepp/assign_addr.hpp"
#include "pas/operations/pepp/bytes.hpp"

class PasOpsPepp_ToBytes : public QObject {
    Q_OBJECT
private slots:
    void smoke(){
        QFETCH(QString, source);
        QFETCH(QList<quint8>, bytes);
        auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(source, nullptr);
        auto str = parsed.errors.join("\n").toStdString();
        QVERIFY2(!parsed.hadError, str.data());
        pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
        auto actualBytes = pas::ops::pepp::toBytes<pas::isa::Pep10ISA>(*parsed.root);
        QCOMPARE(actualBytes, bytes);
    }

    void smoke_data(){
        // Bug: Fix character tests when #115 is resolved.
        QTest::addColumn<QString>("source");
        QTest::addColumn<QList<quint8>>("bytes");

        // Instructions
        QTest::addRow("unary") <<"ASRA\nRET"<< QList<quint8>{0x16,0x00};
        QTest::addRow("nonunary") <<"s:LDWA 0xFAAD,i\nBR s,i"<< QList<quint8>{0x40,0xfa,0xad,0x1c,0x00,0x00};
        // Dot commands
        for (auto &str :
             {".IMPORT s", ".EXPORT s", ".SCALL s", ".USCALL s", ".INPUT s", ".OUTPUT s", ".END"}) {
            QTest::addRow(str) << QString::fromStdString(str) << QList<quint8>{};
        }
        QTest::addRow("BYTE 0xFF") << ".BYTE 0xFF" << QList<quint8>{0xff};
        QTest::addRow("BYTE 254") << ".BYTE 254" << QList<quint8>{0xfe};
        //QTest::addRow("BYTE '\\0xfe'") << ".BYTE '\\xfe'" << QList<quint8>{0xfe};
        QTest::addRow("BYTE -2") << ".BYTE -2" << QList<quint8>{0xfe};

        QTest::addRow("WORD 0xFFFF") << ".WORD 0xFFFF" << QList<quint8>{0xff, 0xff};
        QTest::addRow("WORD 65534") << ".WORD 65534" << QList<quint8>{0xff, 0xfe};
        //QTest::addRow("WORD '\\0xfe'") << ".WORD '\\xfe'" << QList<quint8>{0x00, 0xfe};
        QTest::addRow("WORD -2") << ".WORD -2" << QList<quint8>{0xff, 0xfe};

        QTest::addRow("BLOCK 0") << ".BLOCK 0" << QList<quint8>{};
        QTest::addRow("BLOCK 1") << ".BLOCK 1" << QList<quint8>{0x00};
        QTest::addRow("BLOCK 2") << ".BLOCK 2" << QList<quint8>{0x00, 0x00};
        QTest::addRow("BLOCK 3") << ".BLOCK 3" << QList<quint8>{0x00, 0x00, 0x00};

        QTest::addRow("ASCII short string: no escaped") << ".ASCII \"hi\"" << QList<quint8>{0x68, 0x69};
        QTest::addRow("ASCII short string: 1 escaped") << ".ASCII\".\\n\"" << QList<quint8>{0x2E, 0x0a};
        QTest::addRow("ASCII short string: 2 escaped") << ".ASCII \"\\r\\n\"" << QList<quint8>{0x0d, 0x0a};
        QTest::addRow("ASCII short string: 2 hex") << ".ASCII \"\\xff\\x00\"" << QList<quint8>{0xff, 0x00};
        QTest::addRow("ASCII long string: no escaped") << ".ASCII \"ahi\"" << QList<quint8>{0x61, 0x68, 0x69};
        QTest::addRow("ASCII long string: 1 escaped") << ".ASCII\".a.\\n\"" << QList<quint8>{0x2e, 0x61, 0x2e, 0x0a};
        QTest::addRow("ASCII long string: 2 escaped") << ".ASCII\"a\\r\\n\"" << QList<quint8>{0x61, 0x0d, 0x0a};
        QTest::addRow("ASCII long string: 2 hex") << ".ASCII \"a\\xff\\x00\"" << QList<quint8>{0x61, 0xff, 0x00};

        QTest::addRow("ALIGN 1 @ 0") << ".ALIGN 1" << QList<quint8>{};
        QTest::addRow("ALIGN 2 @ 0") << ".ALIGN 2" << QList<quint8>{};
        QTest::addRow("ALIGN 2 @ 1") << ".BYTE 0xFF\n.ALIGN 2" << QList<quint8>{0xFF, 0x00};
        QTest::addRow("ALIGN 4 @ 0") << ".ALIGN 4" << QList<quint8>{};
        QTest::addRow("ALIGN 4 @ 1") << ".BYTE 0xFF\n.ALIGN 4" << QList<quint8>{0xFF, 0x00, 0x00, 0x00};
        QTest::addRow("ALIGN 4 @ 2") << ".WORD 0xFFAA\n.ALIGN 4" << QList<quint8>{0xFF, 0xAA, 0x00, 0x00};
        QTest::addRow("ALIGN 8 @ 0") << ".ALIGN 8" << QList<quint8>{};
        QTest::addRow("ALIGN 8 @ 1") << ".BYTE 0xFF\n.ALIGN 8" << QList<quint8>{0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        QTest::addRow("ALIGN 8 @ 2") << ".WORD 0xFFAA\n.ALIGN 8" << QList<quint8>{0xFF, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    }
};

#include "to_bytes.test.moc"

QTEST_MAIN(PasOpsPepp_ToBytes)
