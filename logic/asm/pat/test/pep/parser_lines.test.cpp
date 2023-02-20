#include <QObject>
#include <QTest>

#include "pat/pep/parse/rules_lines.hpp"
using namespace std::literals::string_literals;
class PatPepParserLines : public QObject {
  Q_OBJECT
private slots:
  void testBlank() {
    auto value = "  \t"s;
    pat::pep::parse::BlankType result;
    QVERIFY(parse(value.begin(), value.end(), pat::pep::parse::blank, result));
  }
  void testComment() {
    auto value = ";magic"s;
    pat::pep::parse::CommentType result;
    QVERIFY(
        parse(value.begin(), value.end(), pat::pep::parse::comment, result));
  }

  void testUnary() {
    auto values = QList<std::string>{"hi"s, "symbol:hi", "symbol:hi;comment",
                                     "a;comment"};
    for (const auto &value : values) {
      pat::pep::parse::UnaryType result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::unary, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment) << result.hasComment
                 << QString::fromStdString(result.identifier)
                 << QString::fromStdString(result.symbol);
    }
  }

  void testNonUnary() {
    auto values =
        QList<std::string>{"hi 10"s, "symbol:hi 10", "symbol:hi 10;comme nt",
                           "a 10;comment", "hi 10,s"};
    for (const auto &value : values) {
      pat::pep::parse::NonUnaryType result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::nonunary, result),
          value.data());
      qWarning()
          << QString::fromStdString(result.comment)
          << QString::fromStdString(result.identifier) << result.hasComment
          << (boost::get<pat::pep::parse::DecimalLiteral>(result.arg).value)
          << QString::fromStdString(result.addr)
          << QString::fromStdString(result.symbol);
    }
  }
  void testDirective() {
    auto values =
        QList<std::string>{"sym:.hi", "sym: .hi", ".hi 10", ".hi 10, 20"};
    for (const auto &value : values) {
      pat::pep::parse::DirectiveType result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::directive, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.hasComment << result.args.size()
                 << QString::fromStdString(result.symbol);
    }
  }
  void testMacro() {
    auto values =
        QList<std::string>{"sym:@hi", "sym: @hi", "@hi 10", "@hi 10, 20"};
    for (const auto &value : values) {
      pat::pep::parse::MacroType result;
      QVERIFY2(
          parse(value.begin(), value.end(), pat::pep::parse::macro, result),
          value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.hasComment << result.args.size()
                 << QString::fromStdString(result.symbol);
    }
  }
};

#include "parser_lines.test.moc"

QTEST_MAIN(PatPepParserLines);
