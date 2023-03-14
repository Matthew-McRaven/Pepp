#include "pas/operations/pepp/register_system_calls.hpp"
#include "macro/macro.hpp"
#include "macro/registered.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
class PasOpsPepp_RegisterSystemCalls : public QObject {
  Q_OBJECT
private slots:
  // Passes
  void unary() {
    auto registry = QSharedPointer<macro::Registry>::create();
    QString body = ".USCALL s";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    QVERIFY(pas::ops::pepp::registerSystemCalls(*ret.root, registry));
    QVERIFY(registry->contains("s"));
    auto macro = registry->findMacro("s");
    QCOMPARE(macro->type(), macro::types::System);
    auto contents = macro->contents();
    QCOMPARE(contents->name(), u"s"_qs);
    QCOMPARE(contents->argCount(), 0);
    QCOMPARE(contents->body(), u"LDWT s, i\nUSCALL\n"_qs);
  }
  void nonunary() {
    auto registry = QSharedPointer<macro::Registry>::create();
    QString body = ".SCALL s";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    QVERIFY(pas::ops::pepp::registerSystemCalls(*ret.root, registry));
    QVERIFY(registry->contains("s"));
    auto macro = registry->findMacro("s");
    QCOMPARE(macro->type(), macro::types::System);
    auto contents = macro->contents();
    QCOMPARE(contents->name(), u"s"_qs);
    QCOMPARE(contents->argCount(), 2);
    QCOMPARE(contents->body(), u"LDWT s, i\nSCALL %1, %2\n"_qs);
  }
  // Fails
  void duplicates() {
    auto registry = QSharedPointer<macro::Registry>::create();
    QString body = ".SCALL s\n.USCALL s";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(!pas::ops::pepp::registerSystemCalls(*ret.root, registry));
  }
};

#include "register_system_calls.test.moc"

QTEST_MAIN(PasOpsPepp_RegisterSystemCalls)
