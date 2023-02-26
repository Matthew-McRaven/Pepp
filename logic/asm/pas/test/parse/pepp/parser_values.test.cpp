#include <QObject>
#include <QTest>

#include "pas/parse/pepp/rules_values.hpp"
using namespace pas::parse::pepp;
using namespace std::literals::string_literals;
class PasParserPeppValues : public QObject {
  Q_OBJECT
private slots:
  /*void parseSuccess() {
    auto str = "@deci"s;
    auto r = parse_macro(str.begin(), str.end());
    QVERIFY(r);
  }*/
  void parseChLit() {
    auto value = "'\\r'"s;
    CharacterLiteral result;
    auto r = parse(value.begin(), value.end(), character, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseStrLit() {
    auto value = "\"m\\xaa\\xdd\""s;
    StringLiteral result;
    auto r = parse(value.begin(), value.end(), strings, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseIdent() {
    auto value = "a"s;
    Identifier result;
    auto r = parse(value.begin(), value.end(), identifier, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseSignedNumber() {
    auto value = "-1025"s;
    DecimalLiteral result;
    auto r = parse(value.begin(), value.end(), signed_decimal, result);
    qDebug() << (qint64)result.value;
    QVERIFY(r);
  }

  void parseUnsignedNumber() {
    auto value = "1025"s;
    DecimalLiteral result;
    auto r = parse(value.begin(), value.end(), unsigned_decimal, result);
    qDebug() << result.value;
    QVERIFY(r);
  }

  void parseHex() {
    auto value = "0xcade"s;
    HexadecimalLiteral result;
    auto r = parse(value.begin(), value.end(), hexadecimal, result);
    qDebug() << result.value;
    QVERIFY(r);
  }

  void parseAny() {
    auto value = "0xcade"s;
    Value result;
    auto r = parse(value.begin(), value.end(), argument,
                                          result);
    QVERIFY(r);
    qDebug() << boost::get<HexadecimalLiteral>(result).value;
  }
};

#include "parser_values.test.moc"
QTEST_MAIN(PasParserPeppValues)
