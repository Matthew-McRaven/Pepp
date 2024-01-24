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

#include "asm/pas/driver/pep10.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "obj/memmap.hpp"
#include "obj/mmio.hpp"
#include "asm/pas/obj/pep10.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "asm/pas/operations/pepp/string.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/pep10/isa3/system.hpp"
#include <QObject>
#include <QTest>

static const auto is_diskIn = [](const auto &x) {
  return x.name == "diskIn" && x.direction == obj::IO::Direction::kInput &&
         x.minOffset == 0xFFFC && x.maxOffset == 0xFFFC;
};
static const auto is_charIn = [](const auto &x) {
  return x.name == "charIn" && x.direction == obj::IO::Direction::kInput &&
         x.minOffset == 0xFFFD && x.maxOffset == 0xFFFD;
};
static const auto is_charOut = [](const auto &x) {
  return x.name == "charOut" && x.direction == obj::IO::Direction::kOutput &&
         x.minOffset == 0xFFFE && x.maxOffset == 0xFFFE;
};
static const auto is_pwrOff = [](const auto &x) {
  return x.name == "pwrOff" && x.direction == obj::IO::Direction::kOutput &&
         x.minOffset == 0xFFFF && x.maxOffset == 0xFFFF;
};

class PasE2E_Pep10 : public QObject {
  Q_OBJECT
  QSharedPointer<const builtins::Book> book;
  void loadBookMacros(QSharedPointer<macro::Registry> registry) {
    for (auto &macro : book->macros())
      registry->registerMacro(macro::types::Core, macro);
  }

  QStringList nonunary = {"DECI", "CHARI", "CHARO", "STRO", "DECO", "PRINTF"};

  void injectFakeSCallMacros(QSharedPointer<macro::Registry> registry) {
    for (auto &macro : nonunary)
      registry->registerMacro(
          macro::types::Core,
          QSharedPointer<macro::Parsed>::create(
              macro, 2, "LDWA 0,i\nSCALL $1, $2", "pep/10"));
  }

private slots:
  void standalone() {
    QFETCH(QString, chapter);
    QFETCH(QString, figure);
    QFETCH(QString, body);
    QFETCH(bool, isOS);

    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    loadBookMacros(registry);
    if (!isOS)
      injectFakeSCallMacros(registry);

    auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
        {{body, {.isOS = isOS, .ignoreUndefinedSymbols = !isOS}}}, registry);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
    auto target = pipeline->pipelines[0].first;
    QVERIFY(target->bodies.contains(pas::driver::repr::Nodes::name));
    auto root = target->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
    // print out error messages before failing -- enables debugging broken
    // tests.
    if (!result) {
      auto errors = pas::ops::generic::collectErrors(*root);
      for (auto &error : errors)
        qCritical() << error.first.value.line << error.second.message;
    }

    QVERIFY(result);
  }

  void standalone_data() {
    QTest::addColumn<QString>("chapter");
    QTest::addColumn<QString>("figure");
    QTest::addColumn<QString>("body");
    QTest::addColumn<bool>("isOS");
    auto registry = builtins::Registry(nullptr);
    this->book = registry.findBook("Computer Systems, 6th Edition");
    QVERIFY(!book.isNull());

    for (auto &fig : book->figures()) {
      if (!fig->typesafeElements().contains("pep"))
        continue;
      auto chName = fig->chapterName().toStdString();
      auto figName = fig->figureName().toStdString();
      QTest::addRow("Figure %s.%s", chName.data(), figName.data())
          << fig->chapterName() << fig->figureName()
          << fig->typesafeElements()["pep"]->contents << fig->isOS();
    }
  }

  void unified() {
    QFETCH(QString, figName);
    QFETCH(QString, chapter);
    QFETCH(QString, figure);
    QFETCH(QString, userBody);
    QFETCH(QString, osBody);
    QFETCH(bool, isFullOS);

    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    loadBookMacros(registry);

    auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
        {{osBody, {.isOS = true, .ignoreUndefinedSymbols = false}},
         {userBody, {.isOS = false, .ignoreUndefinedSymbols = false}}},
        registry);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

    // for (auto &x : pipeline->globals->table.keys())
    //   qCritical() << "SYMBOL: " << x;
    QCOMPARE(pipeline->pipelines.size(), 2);

    auto osTarget = pipeline->pipelines[0].first;
    QVERIFY(osTarget->bodies.contains(pas::driver::repr::Nodes::name));
    auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;
    if (pipeline->pipelines[0].first->stage != pas::driver::pep10::Stage::End) {
      for (auto &error : pas::ops::generic::collectErrors(*osRoot))
        qCritical() << "OS:   " << error.second.message;
      QVERIFY(false);
    }
    auto userTarget = pipeline->pipelines[1].first;
    QVERIFY(userTarget->bodies.contains(pas::driver::repr::Nodes::name));
    auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                        .value<pas::driver::repr::Nodes>()
                        .value;
    // print out error messages before failing -- enables debugging broken
    // tests.
    if (!result) {
      auto lines = pas::ops::pepp::formatListing<isa::Pep10>(*userRoot);
      for (auto &line : lines)
        qCritical() << line;
      for (auto &error : pas::ops::generic::collectErrors(*osRoot))
        qCritical() << "OS:   " << error.second.message;
      for (auto &error : pas::ops::generic::collectErrors(*userRoot))
        qCritical() << "USER: " << error.second.message;
    }
    auto elf = pas::obj::pep10::createElf();
    pas::obj::pep10::combineSections(*osRoot);
    pas::obj::pep10::writeOS(*elf, *osRoot);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(*elf, *userRoot);
    // Segments are not layed out correctly until saving.
    elf->save(u"%1.%2.elf"_qs.arg(chapter, figure).toStdString());
    elf->load(u"%1.%2.elf"_qs.arg(chapter, figure).toStdString());
    QVERIFY(result);

    // Verify MMIO information.
    auto decs = ::obj::getMMIODeclarations(*elf);
    QCOMPARE(decs.length(), 4);
    QCOMPARE_NE(std::find_if(decs.cbegin(), decs.cend(), is_diskIn),
                decs.cend());
    QCOMPARE_NE(std::find_if(decs.cbegin(), decs.cend(), is_charIn),
                decs.cend());
    QCOMPARE_NE(std::find_if(decs.cbegin(), decs.cend(), is_charOut),
                decs.cend());
    QCOMPARE_NE(std::find_if(decs.cbegin(), decs.cend(), is_pwrOff),
                decs.cend());

    auto buf = ::obj::getMMIBuffers(*elf);
    QCOMPARE(buf.size(), 1);
    auto memMap = obj::getLoadableSegments(*elf);
    auto mergeMap = obj::mergeSegmentRegions(memMap);
    if (isFullOS) {
      QCOMPARE(mergeMap.size(), 3);
      // user memory + system stack
      mergeMap[0].segs = {};
      auto uMem = obj::MemoryRegion{
          .r = 1, .w = 1, .minOffset = 0, .maxOffset = 0xfaf9};
      QCOMPARE(mergeMap[0], uMem);
      // OS text
      mergeMap[1].segs = {};
      auto txt = obj::MemoryRegion{
          .r = 1, .w = 0, .minOffset = 0xfafa, .maxOffset = 0xfff9};
      QCOMPARE(mergeMap[1], txt);
      // Carveout for MMIO
      mergeMap[2].segs = {};
      auto mmio = obj::MemoryRegion{
          .r = 1, .w = 1, .minOffset = 0xfffa, .maxOffset = 0xffff};
      QCOMPARE(mergeMap[2], mmio);
    }
    QSharedPointer<targets::pep10::isa::System> sys;
    QVERIFY_THROWS_NO_EXCEPTION([&sys, &elf]() {
      sys = targets::pep10::isa::systemFromElf(*elf, true);
    }());
    QVector<quint8> dump(0x1'00'00);
    sys->bus()->dump({dump.data(), std::size_t(dump.size())});
    QFile memDump(u"%1.mem.bin"_qs.arg(figName));
    if (memDump.open(QFile::WriteOnly)) {
      memDump.write(reinterpret_cast<const char *>(dump.constData()),
                    dump.size());
      memDump.close();
    }

    auto bootFlg = ::obj::getBootFlagsAddress(*elf);
    auto systemBootFlg = sys->getBootFlagAddress();
    QCOMPARE(bootFlg.has_value(), systemBootFlg.has_value());
    if (bootFlg) {
      QCOMPARE(*bootFlg, 0xFA36);
      QCOMPARE(*systemBootFlg, *bootFlg);
      QCOMPARE(sys->getBootFlags(), 3);
    }
  }
  void unified_data() {
    QTest::addColumn<QString>("figName");
    QTest::addColumn<QString>("chapter");
    QTest::addColumn<QString>("figure");
    QTest::addColumn<QString>("userBody");
    QTest::addColumn<QString>("osBody");
    QTest::addColumn<bool>("isFullOS");
    auto registry = builtins::Registry(nullptr);
    this->book = registry.findBook("Computer Systems, 6th Edition");
    QVERIFY(!book.isNull());

    for (auto &fig : book->figures()) {
      auto defaultOS = fig->defaultOS();
      if (fig->isOS())
        continue;
      else if (!fig->typesafeElements().contains("pep"))
        continue;
      else if (defaultOS == nullptr)
        continue;
      else if (!defaultOS->typesafeElements().contains("pep"))
        continue;
      auto chName = fig->chapterName().toStdString();
      auto figName = fig->figureName().toStdString();
      QTest::addRow("Figure %s.%s with OS", chName.data(), figName.data())
          << u"%1.%2"_qs.arg(fig->chapterName()).arg(fig->figureName())
          << fig->chapterName() << fig->figureName()
          << fig->typesafeElements()["pep"]->contents
          << defaultOS->typesafeElements()["pep"]->contents
          << bool(defaultOS->figureName() == "full");
    }
  }
};

#include "pep10.moc"

QTEST_MAIN(PasE2E_Pep10);
