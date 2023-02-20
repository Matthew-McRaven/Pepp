#include <QObject>
#include <QTest>

#include "pat/pep/parse/rules_values.hpp"
using namespace std::literals::string_literals;
class PatPeppParserMacro : public QObject {
  Q_OBJECT
private slots:
  /*void parseSuccess() {
    auto str = "@deci"s;
    auto r = parse_macro(str.begin(), str.end());
    QVERIFY(r);
  }*/
  void parseChLit() {
    using pat::pep::parse::hexadecimal;
    auto value = "'\\r'"s;
    pat::pep::parse::CharacterLiteral result;
    auto r =
        parse(value.begin(), value.end(), pat::pep::parse::character, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseStrLit() {
    using pat::pep::parse::hexadecimal;
    auto value = "\"m\\xaa\\xdd\""s;
    pat::pep::parse::StringLiteral result;
    auto r =
        parse(value.begin(), value.end(), pat::pep::parse::strings, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseIdent() {
    using pat::pep::parse::hexadecimal;
    auto value = "a"s;
    pat::pep::parse::Identifier result;
    auto r =
        parse(value.begin(), value.end(), pat::pep::parse::identifier, result);
    qDebug() << QString::fromStdString(result.value);
    QVERIFY(r);
  }

  void parseNumber() {
    using pat::pep::parse::decimal;
    auto value = "1025"s;
    pat::pep::parse::DecimalLiteral result;
    auto r = parse(value.begin(), value.end(), decimal, result);
    qDebug() << result.value;
    QVERIFY(r);
  }

  void parseHex() {
    using pat::pep::parse::hexadecimal;
    auto value = "0xcade"s;
    pat::pep::parse::HexadecimalLiteral result;
    auto r =
        parse(value.begin(), value.end(), pat::pep::parse::hexadecimal, result);
    qDebug() << result.value;
    QVERIFY(r);
  }

  void parseAny() {
    using pat::pep::parse::argument;
    auto value = "0xcade"s;
    pat::pep::parse::Value result;
    auto r = parse<typeof(value.begin())>(value.begin(), value.end(),
                                          pat::pep::parse::argument, result);
    QVERIFY(r);
    qDebug() << boost::get<pat::pep::parse::HexadecimalLiteral>(result).value;
  }
};

#include "parser_macro.test.moc"
QTEST_MAIN(PatPeppParserMacro)
