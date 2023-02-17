#include "macro/registry.hpp"
#include "macro/macro.hpp"
#include <QTest>
class MacroRegistry : public QObject {
  Q_OBJECT
private slots:
  void registersMacros() {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(registered->contents(), parsed);
  }
  void findByName() {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(registered->contents(), parsed);
    QCOMPARE(reg.findMacro("alpha"), registered);
  }
  void rejectDuplicateNames() {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body", "none");
    auto parsed2 = new macro::Parsed("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(reg.registerMacro(macro::Type::Core, parsed2), nullptr);
  }
  void delineatesTypes() {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body", "none");
    auto parsed2 = new macro::Parsed("beta", 0, "body", "none");
    QVERIFY(reg.registerMacro(macro::Type::Core, parsed) != nullptr);
    QVERIFY(reg.registerMacro(macro::Type::System, parsed2) != nullptr);
    QVERIFY(reg.findMacrosByType(macro::Type::Core).size() == 1);
    QVERIFY(reg.findMacrosByType(macro::Type::System).size() == 1);
    QVERIFY(reg.findMacrosByType(macro::Type::User).size() == 0);
  }
};
#include "registry.test.moc"
QTEST_MAIN(MacroRegistry);
