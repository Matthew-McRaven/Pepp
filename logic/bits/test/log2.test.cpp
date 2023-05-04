#include "bits/operations/log2.hpp"
#include <QTest>
#include <QtCore>

class BitsOps_Log2 : public QObject {
  Q_OBJECT
private slots:
  void ceil_log2() {
    QFETCH(quint64, input);
    QFETCH(quint8, output);
    QCOMPARE(bits::ceil_log2(input), output);
  }
  void ceil_log2_data() {
    QTest::addColumn<quint64>("input");
    QTest::addColumn<quint8>("output");

    QTest::addRow("1") << quint64(1) << quint8(0);
    QTest::addRow("2") << quint64(2) << quint8(1);
    QTest::addRow("3") << quint64(3) << quint8(2);
    QTest::addRow("4") << quint64(4) << quint8(2);
    QTest::addRow("5") << quint64(5) << quint8(3);
    QTest::addRow("6") << quint64(6) << quint8(3);
    QTest::addRow("7") << quint64(7) << quint8(3);
    QTest::addRow("8") << quint64(8) << quint8(3);
    QTest::addRow("16") << quint64(16) << quint8(4);
  }
};

#include "log2.test.moc"

QTEST_MAIN(BitsOps_Log2);
