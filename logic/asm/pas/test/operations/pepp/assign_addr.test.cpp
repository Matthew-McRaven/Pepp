#include "pas/operations/pepp/assign_addr.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/bits/strings.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/pepp/size.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
using pas::ops::pepp::Direction;
void childRange(QSharedPointer<pas::ast::Node> parent, qsizetype index,
                qsizetype start, qsizetype end) {
  QVERIFY(parent->has<pas::ast::generic::Children>());
  auto children = parent->get<pas::ast::generic::Children>().value;
  QVERIFY(children.size() > index);
  auto child = children[index];
  QVERIFY(child->has<pas::ast::generic::Address>());
  auto address = child->get<pas::ast::generic::Address>().value;
  QCOMPARE(address.start, start % 0xFFFF);
  QCOMPARE(address.size, end);
}

typedef void (*testFn)(QSharedPointer<pas::ast::Node>, qsizetype base);

void unary_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 2);
  childRange(root, 0, base + 0, 1);
  childRange(root, 1, base + 1, 1);
}

void nonunary_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 2);
  childRange(root, 0, base + 0, 3);
  childRange(root, 1, base + 3, 3);
}

void size0_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 2);
  // Size 0 directives have no address.
  // childRange(ret.root, 0, base + 0, 0);
  childRange(root, 1, base + 0, 2);
  // childRange(ret.root, 2, base + 2, 0);
}

void ascii2_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 2);
  childRange(root, 0, base + 0, 2);
  childRange(root, 1, base + 2, 2);
}

void ascii3_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 2);
  childRange(root, 0, base + 0, 2);
  childRange(root, 1, base + 2, 3);
}

void equate_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 3);
  childRange(root, 0, 0, 1);
  // EQUATE generates no bytecode, and has no address.
  // childRange(ret.root, 1, 1, 0);
  // childRange(ret.root, 2, 1, 0);
  QVERIFY(children[1]->has<pas::ast::generic::SymbolDeclaration>());
  QCOMPARE(children[1]
               ->get<pas::ast::generic::SymbolDeclaration>()
               .value->value->value()(),
           10);
  QCOMPARE(children[2]
               ->get<pas::ast::generic::SymbolDeclaration>()
               .value->value->value()(),
           10);
}

void org_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 3);
  childRange(root, 0, 0, 3);
  childRange(root, 2, 0x8000, 3);
}

class PasOpsPepp_AssignAddress : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(qsizetype, base);
    QFETCH(bool, useDriver);
    QFETCH(QString, body);
    QFETCH(testFn, validate);
    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline = pas::driver::pep10::pipeline(body, {.isOS = false});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry =
          QSharedPointer<macro::Registry>::create();
      QVERIFY(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
      QCOMPARE(pipelines.pipelines[0].first->stage,
               pas::driver::pep10::Stage::WholeProgramSanity);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
      auto res = parseRoot(body, nullptr);
      QVERIFY(!res.hadError);
      pas::ops::pepp::assignAddresses<Pep10ISA>(*res.root);
      root = res.root;
    }
    validate(root, base);
  }
  void smoke_data() {
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<bool>("useDriver");
    QTest::addColumn<QString>("body");
    QTest::addColumn<testFn>("validate");

    QTest::addRow("unary @ 0: visitor")
        << qsizetype(0) << false << u"rola\nrolx"_qs << &unary_test;
    QTest::addRow("unary @ 0: driver")
        << qsizetype(0) << true << u"rola\nrolx"_qs << &unary_test;

    QTest::addRow("nonunary @ 0: visitor")
        << qsizetype(0) << false << u"adda 0,i\nldwa 0,i"_qs << &nonunary_test;
    QTest::addRow("nonunary @ 0: driver")
        << qsizetype(0) << true << u"adda 0,i\nldwa 0,i"_qs << &nonunary_test;

    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
      QString input = u"%1 s\n.block 2"_qs.arg(QString::fromStdString(str));
      QTest::addRow("%s @ 0: visitor", str)
          << qsizetype(0) << false << input << &size0_test;
      QTest::addRow("%s @ 0: driver", str)
          << qsizetype(0) << true << input << &size0_test;
    }

    QMap<QString, QString> shortArgs = {{"short string, no escaped", "hi"},
                                        {"short string, 1 escaped", ".\\n"},
                                        {"short string, 2 escaped", "\\r\\n"},
                                        {"short string, 2 hex", "\\xff\\x00"}};
    for (auto caseName : shortArgs.keys()) {
      auto input = u".block 2\n.ASCII \"%1\""_qs.arg(shortArgs[caseName]);
      auto caseStr = caseName.toStdString();
      QTest::addRow("%s: visitor", caseStr.data())
          << qsizetype(0) << false << input << &ascii2_test;
      QTest::addRow("%s: driver", caseStr.data())
          << qsizetype(0) << true << input << &ascii2_test;
    }

    QMap<QString, QString> longArgs = {{"long string, no escaped", "ahi"},
                                       {"long string, 1 escaped", "a.\\n"},
                                       {"long string, 2 escaped", "a\\r\\n"},
                                       {"long string, 2 hex", "a\\xff\\x00"}};
    for (auto caseName : longArgs.keys()) {
      auto input = u".block 2\n.ASCII \"%1\""_qs.arg(longArgs[caseName]);
      auto caseStr = caseName.toStdString();
      QTest::addRow("%s: visitor", caseStr.data())
          << qsizetype(0) << false << input << &ascii3_test;
      QTest::addRow("%s: driver", caseStr.data())
          << qsizetype(0) << true << input << &ascii3_test;
    }

    QTest::addRow(".EQUATE @ 0: visitor")
        << qsizetype(0) << false << u".block 1\ns:.EQUATE 10\nn:.EQUATE s"_qs
        << &equate_test;
    QTest::addRow(".EQUATE @ 0: driver")
        << qsizetype(0) << true << u".block 1\ns:.EQUATE 10\nn:.EQUATE s"_qs
        << &equate_test;

    QTest::addRow(".ORG @ 0: visitor")
        << qsizetype(0) << false << u"adda 0,i\n.ORG 0x8000\nldwa 0,i"_qs
        << &org_test;
    QTest::addRow(".ORG @ 0: driver")
        << qsizetype(0) << true << u"adda 0,i\n.ORG 0x8000\nldwa 0,i"_qs
        << &org_test;
  }

  void align() {
    QFETCH(qsizetype, align);
    QFETCH(qsizetype, base);
    QFETCH(bool, useDriver);
    QString body = u".block 1\n.ALIGN %1\n.block 0"_qs.arg(align);
    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline = pas::driver::pep10::pipeline(body, {.isOS = false});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry =
          QSharedPointer<macro::Registry>::create();
      QVERIFY(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
      QCOMPARE(pipelines.pipelines[0].first->stage,
               pas::driver::pep10::Stage::WholeProgramSanity);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
      auto res = parseRoot(body, nullptr);
      QVERIFY(!res.hadError);
      pas::ops::pepp::assignAddresses<Pep10ISA>(*res.root);
      root = res.root;
    }
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    childRange(root, 0, base + 0, 1);
    childRange(root, 1, base + 1, align - 1);
    childRange(root, 2, align, 0);
  }

  void align_data() {
    QTest::addColumn<qsizetype>("align");
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<bool>("useDriver");

    // QTest::addRow("ALIGN 1 @ 0") << qsizetype(1) << qsizetype(0);
    QTest::addRow("ALIGN 2 @ 0: visitor")
        << qsizetype(2) << qsizetype(0) << false;
    QTest::addRow("ALIGN 4 @ 0: visitor")
        << qsizetype(4) << qsizetype(0) << false;
    QTest::addRow("ALIGN 8 @ 0: visitor")
        << qsizetype(8) << qsizetype(0) << false;
    QTest::addRow("ALIGN 2 @ 0: driver")
        << qsizetype(2) << qsizetype(0) << true;
    QTest::addRow("ALIGN 4 @ 0: driver")
        << qsizetype(4) << qsizetype(0) << true;
    QTest::addRow("ALIGN 8 @ 0: driver")
        << qsizetype(8) << qsizetype(0) << true;
  }

  // Don't test macros, they shouldn't survive as nodes into the address
  // assignment stage.
  // Empty lines do not matter.
};

#include "assign_addr.test.moc"

QTEST_MAIN(PasOpsPepp_AssignAddress)
