
#include "symbol/types.hpp"
#include "symbol/value.hpp"
#include <QObject>
#include <QTest>

class SymbolValue : public QObject {
  Q_OBJECT
private slots:
  // Check that bitmasks work.
  void bitmask() {
    symbol::value::MaskedBits start{
        .byteCount = 1, .bitPattern = 0xf, .mask = 0x7};
    symbol::value::MaskedBits end{
        .byteCount = 1, .bitPattern = 0x7, .mask = 0xf};
    QCOMPARE(start, start);
    QCOMPARE_NE(start, end);
    QCOMPARE(start(), end());
  }
  void empty() {
    auto value = symbol::value::Empty(0);

    QVERIFY_THROWS_NO_EXCEPTION(value.value()());
    QCOMPARE(value.value()(), 0);
  }
  void deleted() {
    auto value = symbol::value::Deleted();
    QVERIFY_THROWS_NO_EXCEPTION(value.value()());
    QCOMPARE(value.value()(), 0);
    QCOMPARE(value.type(), symbol::Type::kDeleted);
  }
  // Check that the values on a numeric value can be mutated.
  void numeric() {
    symbol::value::MaskedBits start{
        .byteCount = 1,
        .bitPattern = 30,
        .mask = 0xff,
    };
    symbol::value::MaskedBits end{
        .byteCount = 1,
        .bitPattern = 20,
        .mask = 0xff,
    };
    auto value = symbol::value::Constant(start);
    QVERIFY_THROWS_NO_EXCEPTION(value.value()());
    QCOMPARE(value.value()(), start());
    QVERIFY_THROWS_NO_EXCEPTION(value.setValue(end));
    QCOMPARE(value.value()(), end());
  }
  // Check that the values on a location value can be mutated.
  void location() {
    auto base = 7;
    auto start_offset = 11, end_offset = 13;
    auto value =
        symbol::value::Location(2, 2, base, start_offset, symbol::Type::kCode);
    QCOMPARE(value.value()(), base + start_offset);
    QVERIFY_THROWS_NO_EXCEPTION(value.setOffset(end_offset));
    QCOMPARE(value.value()(), base + end_offset);
    QVERIFY(value.relocatable());
  }
  // Can't test internal or external symbol pointer value here, as it will
  // require a symbol table.
};

#include "values.test.moc"
QTEST_MAIN(SymbolValue)
