#include <QTest>
#include <QtCore>

#include "bits/operations/copy.hpp"
#include "bits/strings.hpp"

class BitsStrings_AsciiHex : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    quint8 src[] = {0x00, 0xFE, 0xED, 0xBE, 0xEF};
    char dst[sizeof(src) * 3];
    QString golden = "00 FE ED BE EF ";
    /*quint8 golden[sizeof(dst)] = {0x30, 0x30, 0x20, 0x46, 0x45,
                                  0x20, 0x45, 0x44, 0x20, 0x42,
                                  0x45, 0x20, 0x45, 0x46, 0x20};*/
    bits::memclr(dst, sizeof(dst));
    QCOMPARE(bits::bytesToAsciiHex(dst, sizeof(dst), src, sizeof(src), true),
             sizeof(dst));
    QString dstStr = QString::fromLocal8Bit(reinterpret_cast<const char *>(dst),
                                            sizeof(dst));
    QCOMPARE(dstStr, golden);
  }
};

#include "strings_ascii_hex.test.moc"

QTEST_MAIN(BitsStrings_AsciiHex);
