
#include "macro/parse.hpp"

#include <QTest>
#include <tuple>
class MacroParser : public QObject {
  Q_OBJECT
private slots:
  void acceptsValidSpaces() {
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 0"_qs)), true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 	0"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 0	"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci  0"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci  0  "_qs)),
             true);
    QCOMPARE(std::get<0>(
                 macro::analyze_macro_definition(u"@deci	0	"_qs)),
             true);
  }
  void rejectsInvalidSpaces() {

    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deco​0​"_qs)),
             false); // 0-width space before and after
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u" @deci 0"_qs)),
             false); // Can't have whitespace before
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@ deci 0"_qs)),
             false); // Can't have whitespace between @ and name
  }
  void handlesDifferentArity() {
    auto _0ar = macro::analyze_macro_definition(u"@deci 0"_qs);
    auto _8ar = macro::analyze_macro_definition(u"@deco 8"_qs);
    QVERIFY(std::get<0>(_0ar));
    QCOMPARE(std::get<1>(_0ar).toUtf8().toStdString(), "deci");
    QCOMPARE(std::get<2>(_0ar), 0);
    QVERIFY(std::get<0>(_8ar));
    QCOMPARE(std::get<1>(_8ar).toUtf8().toStdString(), "deco");
    QCOMPARE(std::get<2>(_8ar), 8);
  }
  void rejectsComments() {
    QCOMPARE(std::get<0>(macro::analyze_macro_definition("@deci 2 ;fail")),
             false);
  }
  // TODO: Can parse a macro with a body
  // TODO: Requires arg count
  // TODO: Requires @
  // TODO: Rejects symbols, other chars.
};
#include "parse.test.moc"

QTEST_MAIN(MacroParser)
