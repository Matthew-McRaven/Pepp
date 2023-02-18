#include <QObject>
#include <QTest>

#include "pat/pep/parse/parser.hpp"
class PatPeppParserDummy : public QObject {
  Q_OBJECT
private slots:
  void parseSuccess() {
    using namespace std::literals::string_literals;
    auto str = "10,20,30"s;
    auto r = parse_numbers(str.begin(), str.end());
    QVERIFY(r);
  }
};

#include "parser_dummy.test.moc"
QTEST_MAIN(PatPeppParserDummy)
