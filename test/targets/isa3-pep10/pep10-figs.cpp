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

#include <catch.hpp>
#include <elfio/elfio.hpp>
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/obj/pep10.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "link/mmio.hpp"
#include "macro/registry.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/dense.hpp"
#include "sim/device/ide.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/isa3/helpers.hpp"
#include "targets/isa3/system.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "utils/bits/strings.hpp"
#include "utils/bits/swap.hpp"

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
  QString bookName = "Computer Systems, 6th Edition";

  auto book = reg.findBook(bookName);
  return book;
}

QSharedPointer<macro::Registry> registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}

struct User {
  QString pep, pepo;
};

void assemble(ELFIO::elfio &elf, QString os, User user, QSharedPointer<macro::Registry> reg) {
  QList<QPair<QString, pas::driver::pep10::Features>> targets = {{os, {.isOS = true}}};
  if (!user.pep.isEmpty())
    targets.push_back({user.pep, {.isOS = false}});
  auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(targets, reg);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
  REQUIRE(result);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  CHECK(pas::ops::generic::collectErrors(*osRoot).size() == 0);
  pas::obj::pep10::combineSections(*osRoot);
  pas::obj::pep10::writeOS(elf, *osRoot);

  if (!user.pep.isEmpty()) {
    auto userTarget = pipeline->pipelines[1].first;
    auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    CHECK(pas::ops::generic::collectErrors(*userRoot).size() == 0);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(elf, *userRoot);
  } else {
    auto asStd = user.pepo.toStdString();
    auto bytes = bits::asciiHexToByte({asStd.data(), asStd.size()});
    REQUIRE(bytes);
    pas::obj::pep10::writeUser(elf, *bytes);
  }
}

QSharedPointer<ELFIO::elfio> smoke(QString os, QString userPep, QString userPepo, QString input, QByteArray output) {
  auto bookReg = builtins::Registry(nullptr);
  // Load book contents, macros.
  auto bookPtr = book(bookReg);
  auto reg = registry(bookPtr, {});
  auto elf = pas::obj::pep10::createElf();
  assemble(*elf, os, {.pep = userPep, .pepo = userPepo}, reg);

  // Need to reload to properly compute segment addresses.
  {
    std::stringstream s;
    elf->save(s);
    s.seekg(0, std::ios::beg);
    elf->load(s);
  }
  // Skip loading, to save on cycles. However, can't skip dispatch, or
  // main's stack will be wrong.
  auto system = targets::isa::systemFromElf(*elf, true);
  system->init();
  REQUIRE(!system.isNull());
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
    system->tick(sim::api2::Scheduler::Mode::Jump);
  }
  CHECK(system->currentTick() != max);
  // TODO: Ensure that pwrOff was written to.
  // Get all charOut values.
  QByteArray actualOut;
  if (auto charOut = system->output("charOut"); !output.isEmpty() && charOut) {
    auto charOutEndpoint = charOut->endpoint();
    charOutEndpoint->set_to_head();
    for (auto next = charOutEndpoint->next_value(); next.has_value(); next = charOutEndpoint->next_value())
      actualOut.push_back(*next);
  }
  CHECK(actualOut == output);
  return elf;
}
} // namespace

const auto IDE_test = "\
;Transfer FEED BEEF to LBA[0].\n\
    LDWA 0,i\n\
    STWA LBA,d\n\
    STBA offLBA,d\n\
    LDWA da,i\n\
    STWA addrDMA,d\n\
    LDWA 4,i\n\
    STWA lenDMA,d\n\
    LDBA 0xCB,i\n\
    STBA ideCMD,d\n\
;Transfer FEED BEEF from LBA[0] to Mem[0].\n\
;LBA, LBA offset, DMA len remain the same.\n\
    LDWA 0,i\n\
    STWA addrDMA,d\n\
    LDBA 0xC9,i\n\
    STBA ideCMD,d\n\
;Erase FEED from LBA[0].\n\
    LDWA 2,i\n\
    STWA lenDMA,d\n\
    LDBA 0x50,i\n\
    STBA ideCMD,d\n\
;Power Off\n\
    LDBA 0xFEED,i\n\
    STBA pwrOff,d\n\
da: .WORD 0xFEED\n\
    .WORD 0xBEEF\n\
";
TEST_CASE("Pep/10 Assembler Assembly", "[scope:asm][kind:e2e][arch:pep10]") {
  using namespace Qt::StringLiterals;
  auto bookReg = builtins::Registry(nullptr);
  auto bookPtr = book(bookReg);
  auto assemblerFig = bookPtr->findFigure("os", "assembler");
  REQUIRE(!assemblerFig.isNull());
  auto os = QString(assemblerFig->typesafeElements()["pep"]->contents).replace(lf, "");
  auto reg = registry(bookPtr, {});
  auto elf = pas::obj::pep10::createElf();
  assemble(*elf, os, {.pep = IDE_test}, reg);
  // Insert IDE MMIO declaration
  obj::addIDEDeclaration(*elf, elf->sections["os.symtab"], "ideCMD");
  auto decls = obj::getMMIODeclarations(*elf);
  CHECK(decls.size() == 4);
  // And ensure that it is the correct size / is present
  auto IDE = std::find_if(decls.begin(), decls.end(), [](auto &x) { return x.type == obj::IO::Type::kIDE; });
  REQUIRE(IDE != decls.end());
  CHECK(IDE->maxOffset - IDE->minOffset + 1 == 8);

  // Need to reload to properly compute segment addresses.
  {
    std::stringstream s;
    elf->save(s);
    s.seekg(0, std::ios::beg);
    elf->load(s);
  }

  // Skip loading, since BM OS does not have an OS-level loader.
  auto system = targets::isa::systemFromElf(*elf, true);
  REQUIRE(!system.isNull());
  elf->save("pep10asmb.elf");
  CHECK(system->ideControllers().size() == 1);
  auto ide = system->ideController("ideCMD");
  CHECK(ide != nullptr);

  system->init();
  auto cpu = dynamic_cast<targets::pep10::isa::CPU *>(system->cpu());
  REQUIRE(cpu != nullptr);
  auto regs = cpu->regs();
  // Force PC to be 0, skipping OS stuff.
  targets::isa::writeRegister<::isa::Pep10, quint8>(regs, isa::Pep10::Register::PC, 0x0000, gs);
  ide->disk()->clear(0);
  // Create an 8-byte temporary buffer.
  quint64 regVal = 7;
  quint8 *tmp = (quint8 *)&regVal;

  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  auto max = 200'000;
  while (system->currentTick() < max && !endpoint->next_value().has_value())
    system->tick(sim::api2::Scheduler::Mode::Jump);
  CHECK(system->currentTick() != max);

  // Ensure MEM has FEED BEEF and LBA has 0000 BEEF
  regVal = 7;
  // Endianess of host does not match guest; easier to use byte arrays!
  quint8 feedbeef[4] = {0xFE, 0xED, 0xBE, 0xEF};
  quint8 beef[4] = {0x00, 0x00, 0xBE, 0xEF};
  REQUIRE_NOTHROW(system->bus()->read(0, {tmp, 4}, gs));
  CHECK(regVal == *(quint32 *)feedbeef);
  regVal = 7;
  REQUIRE_NOTHROW(ide->disk()->read(0, {tmp, 4}, gs));
  CHECK(regVal == *(quint32 *)beef);
}

TEST_CASE("Pep/10 Figure Assembly", "[scope:asm][kind:e2e][arch:pep10]") {
  using namespace Qt::StringLiterals;
  auto bookReg = builtins::Registry(nullptr);
  auto bookPtr = book(bookReg);
  auto figures = bookPtr->figures();
  for (auto &figure : figures) {
    if (!figure->typesafeElements().contains("pep") && !figure->typesafeElements().contains("pepo")) continue;
    else if (figure->isOS()) continue;
    QString userPep = "", userPepo = "";
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
        std::string fname = u"cs6e.%1%2.elf"_s.arg(ch, fig).toStdString();
        elf->save(fname);
      }
      num++;
    }
  }
}
