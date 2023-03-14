#include "pas/bits/operations.hpp"
#include <QObject>
#include <QTest>
using pas::bits::BitOrder;
using pas::bits::copy;
using vu8 = QList<quint8>;

void verify(quint8 *arr, quint16 index, quint8 golden) {
  QCOMPARE(arr[index], golden);
}
class PasBits_Copy : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(quint16, srcLen);
    QFETCH(vu8, srcData);
    QFETCH(BitOrder, srcOrder);
    QFETCH(quint16, destLen);
    QFETCH(BitOrder, destOrder);
    QFETCH(vu8, destGolden);
    auto dest_le = quint64_le{0};
    auto dest_be = quint64_be{0};
    quint8 *dest = nullptr;
    if (destOrder == BitOrder::BigEndian)
      dest = reinterpret_cast<quint8 *>(&dest_be);
    else if (destOrder == BitOrder::LittleEndian)
      dest = reinterpret_cast<quint8 *>(&dest_le);
    auto src = srcData.constData();
    copy(src, srcOrder, srcLen, dest, destOrder, destLen);
    for (int it = 0; it < destLen; it++)
      verify(dest, it, destGolden[it]);
  }
  void smoke_data() {
    QTest::addColumn<quint16>("srcLen");
    QTest::addColumn<vu8>("srcData");
    QTest::addColumn<BitOrder>("srcOrder");
    QTest::addColumn<quint16>("destLen");
    QTest::addColumn<BitOrder>("destOrder");
    QTest::addColumn<vu8>("destGolden");

    // Same length, same endian
    QTest::addRow("same length, big-big, 1-1 byte")
        << quint16(1) << vu8{1} << BitOrder::BigEndian << quint16(1)
        << BitOrder::BigEndian << vu8{01};
    QTest::addRow("same length, big-big, 2-2 byte")
        << quint16(2) << vu8{0xBE, 0xEF} << BitOrder::BigEndian << quint16(2)
        << BitOrder::BigEndian << vu8{0xBE, 0xEF};
    QTest::addRow("same length, little-little, 1-1 byte")
        << quint16(1) << vu8{1} << BitOrder::LittleEndian << quint16(1)
        << BitOrder::LittleEndian << vu8{01};
    QTest::addRow("same length, little-little, 2-2 byte")
        << quint16(2) << vu8{0xEF, 0xBE} << BitOrder::LittleEndian << quint16(2)
        << BitOrder::LittleEndian << vu8{0xEF, 0xBE};

    // Source longer, same endian
    QTest::addRow("source longer, big-big, 3-2 byte")
        << quint16(3) << vu8{0xAA, 0xBB, 0xCC} << BitOrder::BigEndian
        << quint16(2) << BitOrder::BigEndian << vu8{0xBB, 0xCC};
    QTest::addRow("source longer, little-little, 3-2 byte")
        << quint16(3) << vu8{0xCC, 0xBB, 0xAA} << BitOrder::LittleEndian
        << quint16(2) << BitOrder::LittleEndian << vu8{0xCC, 0xBB};

    // Dest longer, same endian
    QTest::addRow("dest longer, big-big, 2-3 byte")
        << quint16(2) << vu8{0xAA, 0xBB} << BitOrder::BigEndian << quint16(3)
        << BitOrder::BigEndian << vu8{0x00, 0xAA, 0xBB};
    QTest::addRow("dest longer, little-little, 2-3 byte")
        << quint16(2) << vu8{0xBB, 0xAA} << BitOrder::LittleEndian << quint16(3)
        << BitOrder::LittleEndian << vu8{0xBB, 0xAA, 0x00};

    // Same length, different endian
    QTest::addRow("same length, little-big, 1-1 byte")
        << quint16(1) << vu8{1} << BitOrder::LittleEndian << quint16(1)
        << BitOrder::BigEndian << vu8{01};
    QTest::addRow("same length, little-big, 2-2 byte")
        << quint16(2) << vu8{0xEF, 0xBE} << BitOrder::LittleEndian << quint16(2)
        << BitOrder::BigEndian << vu8{0xBE, 0xEF};
    QTest::addRow("same length, big-little, 1-1 byte")
        << quint16(1) << vu8{1} << BitOrder::BigEndian << quint16(1)
        << BitOrder::LittleEndian << vu8{01};
    QTest::addRow("same length, big-little, 2-2 byte")
        << quint16(2) << vu8{0xBE, 0xEF} << BitOrder::BigEndian << quint16(2)
        << BitOrder::LittleEndian << vu8{0xEF, 0xBE};

    // Source longer, different endian
    QTest::addRow("source longer, little-big, 3-2 byte")
        << quint16(3) << vu8{0xCC, 0xBB, 0xAA} << BitOrder::LittleEndian
        << quint16(2) << BitOrder::BigEndian << vu8{0xBB, 0xCC};
    QTest::addRow("source longer, big-little, 3-2 byte")
        << quint16(3) << vu8{0xAA, 0xBB, 0xCC} << BitOrder::BigEndian
        << quint16(2) << BitOrder::LittleEndian << vu8{0xCC, 0xBB};

    // Dest longer, different endian
    QTest::addRow("dest longer, little-big, 2-3 byte")
        << quint16(2) << vu8{0xBB, 0xAA} << BitOrder::LittleEndian << quint16(3)
        << BitOrder::BigEndian << vu8{0x00, 0xAA, 0xBB};
    QTest::addRow("dest longer, big-little, 2-3 byte")
        << quint16(2) << vu8{0xAA, 0xBB} << BitOrder::BigEndian << quint16(3)
        << BitOrder::LittleEndian << vu8{0xBB, 0xAA, 0x00};
  }
};

#include "copy.test.moc"

QTEST_MAIN(PasBits_Copy)
