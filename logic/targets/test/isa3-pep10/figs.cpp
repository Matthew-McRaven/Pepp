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

#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "bits/strings.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "macro/registry.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/obj/pep10.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"
#include <elfio/elfio.hpp>

static const auto lf = QRegularExpression("\r");

static const auto rw =
    sim::api::memory::Operation{.speculative = false,
        .kind = sim::api::memory::Operation::Kind::data,
        .effectful = false};

static const auto gs = sim::api::memory::Operation{
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = false,
};
QSharedPointer<const builtins::Book> book(builtins::Registry& reg) {
  QString bookName = "Computer Systems, 6th Edition";

  auto book = reg.findBook(bookName);
  return book;
}

QSharedPointer<macro::Registry>
registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}

struct User {
  QString pep, pepo;
};

void assemble(ELFIO::elfio &elf, QString os, User user,
              QSharedPointer<macro::Registry> reg) {
  QList<QPair<QString, pas::driver::pep10::Features>> targets = {
      {os, {.isOS = true}}};
  if (!user.pep.isEmpty())
    targets.push_back({user.pep, {.isOS = false}});
  auto pipeline = pas::driver::pep10::pipeline<pas::driver::BoostParserTag>(targets, reg);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
  QVERIFY(result);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
      .value<pas::driver::repr::Nodes>()
      .value;
  QCOMPARE(pas::ops::generic::collectErrors(*osRoot).size(), 0);
  pas::obj::pep10::combineSections(*osRoot);
  pas::obj::pep10::writeOS(elf, *osRoot);

  if (!user.pep.isEmpty()) {
    auto userTarget = pipeline->pipelines[1].first;
    auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
        .value<pas::driver::repr::Nodes>()
        .value;
    QCOMPARE(pas::ops::generic::collectErrors(*userRoot).size(), 0);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(elf, *userRoot);
  } else {
    auto asStd = user.pepo.toStdString();
    auto bytes = bits::asciiHexToByte({asStd.data(), asStd.size()});
    QVERIFY(bytes);
    pas::obj::pep10::writeUser(elf, *bytes);
  }
}

class Targets_ISA3Pep10_Figures : public QObject {
Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, os);
    QFETCH(QString, userPep);
    QFETCH(QString, userPepo);
    QFETCH(QString, input);
    QFETCH(QByteArray, output);
    QFETCH(bool, isBM);
    auto bookReg = builtins::Registry(nullptr);
    // Load book contents, macros.
    auto bookPtr = book(bookReg);
    auto reg = registry(bookPtr, {});
    auto elf = pas::obj::pep10::createElf();
    assemble(*elf, os, {.pep = userPep, .pepo = userPepo}, reg);

    // Need to reload to properly compute segment addresses. Store in temp
    // directory to prevent clobbering local file contents.
    {
      QTemporaryDir dir;
      QVERIFY(dir.isValid());
      auto path = dir.filePath("tmp.elf").toStdString();
      elf->save(path);
      elf->load(path);
    }
    // Skip loading, to save on cycles. However, can't skip dispatch, or
    // main's stack will be wrong.
    auto system = targets::pep10::isa::systemFromElf(*elf, isBM);
    system->init();
    QVERIFY(!system.isNull());
    system->setBootFlags(true, true);
    if (auto charIn = system->input("charIn"); !input.isEmpty() && charIn) {
      auto charInEndpoint = charIn->endpoint();
      for (auto c : input.toStdString())
        charInEndpoint->append_value(c);
    }

    // Run until machine terminates.
    auto pwrOff = system->output("pwrOff");
    auto endpoint = pwrOff->endpoint();
    bool fail = false;
    auto max = 200'000;
    while (system->currentTick() < max && !endpoint->next_value().has_value()) {
      auto ret = system->tick(sim::api::Scheduler::Mode::Jump);
      fail |= ret.second.error != sim::api::tick::Error::Success;
      QVERIFY(!fail);
    }
    QCOMPARE_NE(system->currentTick(), max);
    // TODO: Ensure that pwrOff was written to.
    // Get all charOut values.
    QByteArray actualOut;
    if (auto charOut = system->output("charOut");
        !output.isEmpty() && charOut) {
      auto charOutEndpoint = charOut->endpoint();
      charOutEndpoint->set_to_head();
      for (auto next = charOutEndpoint->next_value(); next.has_value();
           next = charOutEndpoint->next_value())
        actualOut.push_back(*next);
    }
    QCOMPARE(actualOut, output);
  }

  void smoke_data() {
    QTest::addColumn<QString>("os");
    QTest::addColumn<QString>("userPep");
    QTest::addColumn<QString>("userPepo");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("output");
    QTest::addColumn<bool>("isBM");
    auto bookReg = builtins::Registry(nullptr);
    auto bookPtr = book(bookReg);
    auto figures = bookPtr->figures();
    for (auto &figure : figures) {
      if (!figure->typesafeElements().contains("pep") &&
          !figure->typesafeElements().contains("pepo"))
        continue;
      else if (figure->isOS())
        continue;
      QString userPep = "", userPepo = "";
      if (figure->typesafeElements().contains("pep"))
          userPep = QString(figure->typesafeElements()["pep"]->contents).replace(lf,"");
      else if (figure->typesafeElements().contains("pepo"))
          userPepo = QString(figure->typesafeElements()["pepo"]->contents).replace(lf, "");
      auto os = QString(figure->defaultOS()->typesafeElements()["pep"]->contents).replace(lf, "");
      bool isBM = !os.contains("bootFlg");
      auto ch = figure->chapterName(), fig = figure->figureName();
      int num = 0;
      for (auto io : figure->typesafeTests()) {
        auto name = u"Figure %1.%2 on IO %3"_qs.arg(ch).arg(fig).arg(num);
        auto nameAsStd = name.toStdString();

        QString input = io->input.toString().replace(lf, "");
        QByteArray output = io->output.toString().replace(lf,"").toUtf8();
        QTest::addRow(nameAsStd.c_str())
            << os << userPep << userPepo << input << output << isBM;
        num++;
      }
    }
  }
};

#include "figs.moc"

QTEST_MAIN(Targets_ISA3Pep10_Figures)
