#include "pas/operations/pepp/addressable.hpp"
#include "isa/pep10.hpp"
#include "pas/ast/node.hpp"
#include "pas/driver/pep10.hpp"
#include <QObject>
#include <QTest>

class PasOpsPepp_Addressable : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, body);
    QFETCH(bool, addressable);
    auto pipeline = pas::driver::pep10::pipeline({{body, {.isOS = true}}});
    QVERIFY(pipeline->assemble(pas::driver::pep10::Stage::Parse));
    auto target = pipeline->pipelines[0].first;
    QVERIFY(!target.isNull());
    QVERIFY(target->bodies.contains(pas::driver::repr::Nodes::name));
    auto root = target->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
    auto children = pas::ast::children(*root);
    QCOMPARE(children.size(), 1);
    QCOMPARE(pas::ops::pepp::isAddressable<isa::Pep10>(*children[0]),
             addressable);
  }
  void smoke_data() {
    QTest::addColumn<QString>("body");
    QTest::addColumn<bool>("addressable");

    QTest::addRow("blank") << u""_qs << false;
    QTest::addRow("comment") << u";hi"_qs << false;

    QTest::addRow("unary") << u"ret"_qs << true;
    QTest::addRow("nonunary branch") << u"br 20"_qs << true;
    QTest::addRow("nonunary nonbranch") << u"ldwa 0,i"_qs << true;

    QTest::addRow(".ALIGN") << u".ALIGN 2"_qs << true;
    QTest::addRow(".ASCII") << u".ASCII \"hi\""_qs << true;
    QTest::addRow(".BLOCK") << u".BLOCK 2"_qs << true;
    QTest::addRow(".BURN") << u".BURN 0xFFFF"_qs << false;
    QTest::addRow(".BYTE") << u".BYTE 0xff"_qs << true;
    QTest::addRow(".END") << u".END"_qs << false;
    QTest::addRow(".EQUATE") << u"s:.EQUATE 10"_qs << false;
    QTest::addRow(".EXPORT") << u".EXPORT s"_qs << false;
    QTest::addRow(".IMPORT") << u".IMPORT s"_qs << false;
    QTest::addRow(".INPUT") << u".INPUT s"_qs << false;
    QTest::addRow(".OUTPUT") << u".OUTPUT s"_qs << false;
    QTest::addRow(".ORG") << u".ORG 0xFAAD"_qs << false;
    QTest::addRow(".SCALL") << u".SCALL s"_qs << false;
    QTest::addRow(".SECTION") << u".SECTION \".text\""_qs << false;
    QTest::addRow(".USCALL") << u".USCALL s"_qs << false;
    QTest::addRow(".WORD") << u".WORD 10"_qs << true;
  }
};

#include "addressable.test.moc"

QTEST_MAIN(PasOpsPepp_Addressable)
