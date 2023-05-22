#include "macro/registry.hpp"
#include "macro/macro.hpp"
#include "macro/registered.hpp"
#include <QTest>
class MacroRegistry : public QObject {
  Q_OBJECT
private slots:
  void registersMacros() {
    macro::Registry reg;
    auto parsed =
        QSharedPointer<macro::Parsed>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(registered->contents(), parsed);
  }
  void findByName() {
    macro::Registry reg;
    auto parsed =
        QSharedPointer<macro::Parsed>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(registered->contents(), parsed);
    QCOMPARE(reg.findMacro("alpha"), registered);
  }
  void rejectDuplicateNames() {
    macro::Registry reg;
    auto parsed =
        QSharedPointer<macro::Parsed>::create("alpha", 0, "body", "none");
    auto parsed2 =
        QSharedPointer<macro::Parsed>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    QCOMPARE_NE(registered, nullptr);
    QCOMPARE(reg.registerMacro(macro::types::Type::Core, parsed2), nullptr);
  }
  void delineatesTypes() {
    macro::Registry reg;
    auto parsed =
        QSharedPointer<macro::Parsed>::create("alpha", 0, "body", "none");
    auto parsed2 =
        QSharedPointer<macro::Parsed>::create("beta", 0, "body", "none");
    QVERIFY(reg.registerMacro(macro::types::Type::Core, parsed) != nullptr);
    QVERIFY(reg.registerMacro(macro::types::Type::System, parsed2) != nullptr);
    QVERIFY(reg.findMacrosByType(macro::types::Type::Core).size() == 1);
    QVERIFY(reg.findMacrosByType(macro::types::Type::System).size() == 1);
    QVERIFY(reg.findMacrosByType(macro::types::Type::User).size() == 0);
  }
};
#include "registry.moc"
QTEST_MAIN(MacroRegistry);
