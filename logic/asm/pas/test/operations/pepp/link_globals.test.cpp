#include "pas/operations/generic/link_globals.hpp"
#include "pas/driver/common.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
class PasOpsPepp_LinkGlobals : public QObject {
  Q_OBJECT
private slots:
  void linkIntraTree() {
    QString body = "s:.block 10\n.EXPORT s\nLDWA s,i\n.END\n.END";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {u"EXPORT"_qs});
    QVERIFY(globals->contains("s"));
    auto sym = globals->get("s");
    QCOMPARE(sym->binding, symbol::Binding::kGlobal);
  }
};

#include "link_globals.test.moc"

QTEST_MAIN(PasOpsPepp_LinkGlobals)
