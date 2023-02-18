#include <QObject>
#include <QTest>

#include "pat/pep/parse/lines.hpp"
using namespace std::literals::string_literals;
class PatPepParserLines : public QObject {
  Q_OBJECT
private slots:
  void testBlank() {
    auto value = "  \t"s;
    pat::pep::ast::Blank result;
    QVERIFY(parse(value.begin(), value.end(), pat::pep::parse::blank, result));
  }
  void testComment() {
    auto value = ";magic"s;
    pat::pep::ast::Comment result;
    QVERIFY(
        parse(value.begin(), value.end(), pat::pep::parse::comment, result));
  }

  void testUnary() {
    auto values = QList<std::string>{"hi"s, "symbol:hi", "symbol:hi;comment",
                                     "a;comment"};
    for (const auto &value : values) {
      pat::pep::ast::Unary result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::unary, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << QString::fromStdString(result.symbol);
    }
  }

  void testNonUnary() {
    auto values =
        QList<std::string>{"hi 10"s, "symbol:hi 10", "symbol:hi 10;comme nt",
                           "a 10;comment", "hi 10,s"};
    for (const auto &value : values) {
      pat::pep::ast::NonUnary result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::nonunary, result),
          value.data());
      qWarning()
          << QString::fromStdString(result.comment)
          << QString::fromStdString(result.identifier)
          << (boost::get<pat::pep::ast::DecimalLiteral>(result.arg).value)
          << QString::fromStdString(result.addr)
          << QString::fromStdString(result.symbol);
    }
  }
  void testDirective() {
    auto values =
        QList<std::string>{"sym:.hi", "sym: .hi", ".hi 10", ".hi 10, 20"};
    for (const auto &value : values) {
      pat::pep::ast::Directive result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::directive, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.args.size() << QString::fromStdString(result.symbol);
    }
  }
  void testMacro() {
    auto values =
        QList<std::string>{"sym:@hi", "sym: @hi", "@hi 10", "@hi 10, 20"};
    for (const auto &value : values) {
      pat::pep::ast::Macro result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::macro, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.args.size() << QString::fromStdString(result.symbol);
    }
  }
};

#include "parser_lines.test.moc"

QTEST_MAIN(PatPepParserLines);
