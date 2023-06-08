/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/bits/strings.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/pepp/assign_addr.hpp"
#include "pas/operations/pepp/size.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
using pas::ops::pepp::Direction;
void childRange(QSharedPointer<pas::ast::Node> parent, qsizetype index,
                qsizetype start, qsizetype size) {
  QVERIFY(parent->has<pas::ast::generic::Children>());
  auto children = parent->get<pas::ast::generic::Children>().value;
  QVERIFY(children.size() > index);
  auto child = children[index];
  QVERIFY(child->has<pas::ast::generic::Address>());
  auto address = child->get<pas::ast::generic::Address>().value;
  QCOMPARE(address.start, start % 0x10000);
  QCOMPARE(address.size, size);
}
class PasOpsPepp_AssignAddressBurn : public QObject {
  Q_OBJECT
private slots:
  void unary() {
    QFETCH(qsizetype, base);
    QString body = ".BURN 0xFFFF\nrola\nrolx";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 1, 0xFFFE, 1);
    childRange(ret.root, 2, 0xFFFF, 1);
  }
  void unary_data() {
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("0") << qsizetype(0);
    // QTest::addRow("200") << qsizetype(200);
    // QTest::addRow("0xFFFE") << qsizetype(0xFFFE);
  }

  void nonunary() {
    QFETCH(qsizetype, base);
    QString body = ".BURN 0x7FFF\nadda 0,i\nldwa 0,i";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 1, 0x7FFF - 5, 3);
    childRange(ret.root, 2, 0x7FFF - 2, 3);
  }
  void nonunary_data() {
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("0") << qsizetype(0);
    // QTest::addRow("200") << qsizetype(200);
    // QTest::addRow("0xFFFE") << qsizetype(0xFFFE);
  }

  void size0Directives() {
    QFETCH(qsizetype, base);
    QFETCH(QString, directive);
    QString body = u".BURN 0xABCD\n%1 s\n.block 2\n%1 s"_qs.arg(directive);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 4);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);

    childRange(ret.root, 1, 0xABCD - 2, 0);
    childRange(ret.root, 2, 0xABCD - 1, 2);
    childRange(ret.root, 3, 0xABCD, 0);
  }
  void size0Directives_data() {
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<QString>("directive");

    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"})
      QTest::addRow("%s", str) << qsizetype(0) << QString::fromStdString(str);
  }

  void ascii() {
    QFETCH(qsizetype, base);
    QFETCH(QString, arg);
    auto len = pas::bits::escapedStringLength(arg);
    QString body = u".BURN 0x00FF\n.block 2\n.ASCII \"%1\""_qs.arg(arg);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 1, 0xFF - len - 2 + 1, 2);
    childRange(ret.root, 2, 0xFF - len + 1, len);
  }
  void ascii_data() {
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<QString>("arg");
    QTest::addRow("short string: no escaped") << qsizetype(0) << "hi";
    QTest::addRow("short string: 1 escaped") << qsizetype(0) << ".\\n";
    QTest::addRow("short string: 2 escaped") << qsizetype(0) << "\\r\\n";
    QTest::addRow("short string: 2 hex") << qsizetype(0) << "\\xff\\x00";
    QTest::addRow("long string: no escaped") << qsizetype(0) << "ahi";
    QTest::addRow("long string: 1 escaped") << qsizetype(0) << "a.\\n";
    QTest::addRow("long string: 2 escaped") << qsizetype(0) << "a\\r\\n";
    QTest::addRow("long string: 2 hex") << qsizetype(0) << "a\\xff\\x00";
  }

  void align() {
    QFETCH(qsizetype, align);
    QFETCH(qsizetype, base);
    QString body = u".BURN 0xFFFF\n .block 1\n.ALIGN %1"_qs.arg(align);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 1, base, 1);
    childRange(ret.root, 2, base + (align > 1 ? 1 : 0), 0xFFFF - base);
  }
  void align_data() {
    QTest::addColumn<qsizetype>("align");
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("ALIGN 1 @ 0") << qsizetype(1) << qsizetype(0xFFFF);
    QTest::addRow("ALIGN 2 @ 0") << qsizetype(2) << qsizetype(0xFFFE);
    QTest::addRow("ALIGN 4 @ 0") << qsizetype(4) << qsizetype(0xFFFC);
    QTest::addRow("ALIGN 8 @ 0") << qsizetype(8) << qsizetype(0xFFF8);
  }
  void equate() {
    QString body = ".BURN 0xFFFF\ny:.block 1\ns:.EQUATE 10\nn:.EQUATE y";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    auto errors = ret.errors.join("\n").toStdString();
    QVERIFY2(!ret.hadError, errors.data());
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 4);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 1, 0xFFFF, 1);
    QVERIFY(children[2]->has<pas::ast::generic::SymbolDeclaration>());
    QCOMPARE(children[2]
                 ->get<pas::ast::generic::SymbolDeclaration>()
                 .value->value->value()(),
             10);
    QVERIFY(children[3]->has<pas::ast::generic::SymbolDeclaration>());
    QCOMPARE(children[3]
                 ->get<pas::ast::generic::SymbolDeclaration>()
                 .value->value->value()(),
             0xFFFF);
  }
  // Don't test macros, they shouldn't survive as nodes into the address
  // assignment stage.
  // Empty lines do not matter.
};

#include "assign_addr_burn.moc"

QTEST_MAIN(PasOpsPepp_AssignAddressBurn)
