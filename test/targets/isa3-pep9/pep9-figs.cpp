/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include <QStringLiteral>
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include "asm/pas/driver/pep9.hpp"
#include "asm/pas/obj/pep9.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "utils/bits/swap.hpp"
#include "utils/bits/strings.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "helpers/asmb.hpp"
#include "link/mmio.hpp"
#include "macro/registry.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/dense.hpp"
#include "sim/device/ide.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/isa3/helpers.hpp"
#include "targets/isa3/system.hpp"
#include "targets/pep9/isa3/cpu.hpp"

namespace {
static const auto lf = QRegularExpression("\r");

static const auto rw = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

static const auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};

QSharedPointer<const builtins::Book> book(builtins::Registry &reg) {
  QString bookName = "Computer Systems, 5th Edition";

  auto book = reg.findBook(bookName);
  return book;
}

QSharedPointer<macro::Registry> registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros()) macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}

struct User {
  QString pep, pepo;
};

QSharedPointer<ELFIO::elfio> assemble(QString os, User user, QSharedPointer<macro::Registry> reg) {
  helpers::AsmHelper helper(reg, os, pepp::Architecture::PEP9);
  if (!user.pep.isEmpty()) helper.setUserText(user.pep);
  CHECK(helper.assemble());
  CHECK(helper.errors().isEmpty());
  QSharedPointer<ELFIO::elfio> elf;
  if (!user.pepo.isEmpty()) {
    auto asStd = user.pepo.toStdString();
    auto bytes = bits::asciiHexToByte({asStd.data(), asStd.size()});
    elf = helper.elf(bytes);
  } else elf = helper.elf();
  return elf;
}

QSharedPointer<ELFIO::elfio> smoke(QString os, QString userPep, QString userPepo, QString input, QByteArray output) {
  auto bookReg = builtins::Registry(nullptr);
  // Load book contents, macros.
  auto bookPtr = book(bookReg);
  auto reg = registry(bookPtr, {});
  QSharedPointer<ELFIO::elfio> elf = nullptr;
  REQUIRE_NOTHROW(elf = assemble(os, {.pep = userPep, .pepo = userPepo}, reg));

  // Skip loading, to save on cycles.
  auto system = targets::isa::systemFromElf(*elf, true);
  REQUIRE(!system.isNull());
  system->init();
  if (auto charIn = system->input("charIn"); !input.isEmpty() && charIn) {
    auto charInEndpoint = charIn->endpoint();
    for (auto c : input.toStdString()) charInEndpoint->append_value(c);
  }

  // Run until machine terminates.
  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  auto max = 200'000;
  while (system->currentTick() < max && !endpoint->next_value().has_value()) {
    system->tick(sim::api2::Scheduler::Mode::Jump);
  }
  CHECK(system->currentTick() < max);
  QByteArray actualOut;
  if (auto charOut = system->output("charOut"); !output.isEmpty() && charOut) {
    auto charOutEndpoint = charOut->endpoint();
    charOutEndpoint->set_to_head();
    for (auto next = charOutEndpoint->next_value(); next.has_value(); next = charOutEndpoint->next_value())
      actualOut.push_back(*next);
  }
  CHECK(std::string(actualOut) == std::string(output));
  return elf;
}
} // namespace

TEST_CASE("Pep/9 Figure Assembly", "[scope:asm][kind:e2e][arch:pep9]") {
  using namespace Qt::StringLiterals;
  auto bookReg = builtins::Registry(nullptr);
  auto bookPtr = book(bookReg);
  auto figures = bookPtr->figures();
  for (auto &figure : figures) {
    // if (!(figure->chapterName() == "06" && figure->figureName() == "08")) continue;
    // if (!(figure->chapterName() == "06" && figure->figureName() == "25")) continue;
    if (!figure->typesafeElements().contains("pep") && !figure->typesafeElements().contains("pepo")) continue;
    else if (figure->isOS()) continue;
    QString userPep = {}, userPepo = {};
    if (figure->typesafeElements().contains("pep"))
      userPep = QString(figure->typesafeElements()["pep"]->contents).replace(lf, "");
    else if (figure->typesafeElements().contains("pepo"))
      userPepo = QString(figure->typesafeElements()["pepo"]->contents).replace(lf, "");
    auto os = QString(figure->defaultOS()->typesafeElements()["pep"]->contents).replace(lf, "");
    auto ch = figure->chapterName(), fig = figure->figureName();
    int num = 0;
    for (auto io : figure->typesafeTests()) {
      auto name = u"Figure %1.%2 on IO %3"_s.arg(ch).arg(fig).arg(num);
      auto nameAsStd = name.toStdString();

      QString input = io->input.toString().replace(lf, "");
      QByteArray output = io->output.toString().replace(lf, "").toUtf8();
      DYNAMIC_SECTION(nameAsStd << " on: " << input.toStdString()) {
        auto elf = smoke(os, userPep, userPepo, input, output);
        std::string fname = u"cs5e.%1%2.elf"_s.arg(ch, fig).toStdString();
        elf->save(fname);
      }
      num++;
    }
  }
}
