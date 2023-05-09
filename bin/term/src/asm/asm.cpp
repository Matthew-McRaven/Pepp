#include "asm.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/obj/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include "pas/operations/pepp/string.hpp"
#include <iostream>

AsmTask::AsmTask(int ed, std::string userFname, std::string out,
                 QObject *parent)
    : Task(parent), ed(ed), userIn(userFname), pepoOut(out) {}

void AsmTask::setOsFname(std::string fname) { osIn = fname; }

void AsmTask::setOsListingFname(std::string fname) { osListOut = fname; }

void AsmTask::emitElfTo(std::string fname) { elfOut = fname; }

void AsmTask::run() {
  auto bookRegistry = builtins::Registry(nullptr);
  auto macroRegistry = QSharedPointer<macro::Registry>::create();
  auto book = bookRegistry.findBook("Computer Systems, 6th Edition");
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(macro::types::Core, macro);
  QString userContents;
  {
    QFile uIn(QString::fromStdString(userIn)); // auto-closes
    uIn.open(QIODevice::ReadOnly | QIODevice::Text);
    userContents = uIn.readAll();
  }

  // If no OS, default to full.
  QString osContents;
  if (osIn->empty()) {
    auto os = book->findFigure("os", "full");
    osContents = os->typesafeElements()["pep"]->contents;
  } else {
    QFile oIn(QString::fromStdString(*osIn)); // auto-closes
    oIn.open(QIODevice::ReadOnly | QIODevice::Text);
    osContents = oIn.readAll();
  }
  auto pipeline = pas::driver::pep10::pipeline(
      {{osContents, {.isOS = true}}, {userContents, {.isOS = false}}},
      macroRegistry);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  auto userTarget = pipeline->pipelines[1].first;
  auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;
  if (!result) {
    std::cerr << "Assembly failed, check error log" << std::endl;
    QFileInfo err(QString::fromStdString(userIn));
    QString errFName = err.path() + "/" + err.completeBaseName() + ".err.txt";
    QFile errF(errFName);
    if (errF.open(QIODevice::WriteOnly | QIODevice::Truncate |
                  QIODevice::Text)) {
      bool hadOsErr = false;
      auto ts = QTextStream(&errF);
      auto osErrors = pas::ops::generic::collectErrors(*osRoot);
      auto userErrors = pas::ops::generic::collectErrors(*userRoot);
      if (!osErrors.empty()) {
        ts << "OS Errors:\n";
        auto splitOS = osContents.split("\n");
        for (const auto &err : osErrors) {
          ts << ";Line " << err.first.value.line + 1 << "\n";
          ts << splitOS[err.first.value.line]
             << " ;ERROR: " << err.second.message << "\n";
        }
        ts << "User Errors:\n";
      }
      if (!userErrors.empty()) {
        auto splitUser = userContents.split("\n");
        for (const auto &err : userErrors) {
          ts << ";Line " << err.first.value.line + 1 << "\n";
          ts << splitUser[err.first.value.line]
             << " ;ERROR: " << err.second.message << "\n";
        }
      }
      errF.close();
    } else
      std::cerr << "Failed to open error log for writing: "
                << errFName.toStdString() << std::endl;
    return emit finished(1);
  }

  // Assembly succeded!

  if (elfOut.has_value()) {
    auto elf = pas::obj::pep10::createElf();
    pas::obj::pep10::combineSections(*osRoot);
    pas::obj::pep10::writeOS(elf, *osRoot);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(elf, *userRoot);
    elf.save(elfOut.value());
  }

  try {
    auto lines = pas::ops::pepp::formatListing<isa::Pep10>(*userRoot);
    QFileInfo pepl(QString::fromStdString(userIn));
    QString peplFName = pepl.path() + "/" + pepl.completeBaseName() + ".pepl";
    QFile peplF(peplFName);
    if (peplF.open(QIODevice::WriteOnly | QIODevice::Truncate |
                   QIODevice::Text)) {
      auto ts = QTextStream(&peplF);
      for (auto &line : lines)
        ts << line << "\n";
    } else
      std::cerr << "Failed to open listing for writing: "
                << peplFName.toStdString() << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Failed to generate user listing due to internal bug.\n";
  }
  if (osListOut) {
    try {
      auto lines = pas::ops::pepp::formatListing<isa::Pep10>(*osRoot);
      QFile peplF(QString::fromStdString(*osListOut));
      if (peplF.open(QFile::OpenModeFlag::WriteOnly)) {
        auto ts = QTextStream(&peplF);
        for (auto &line : lines)
          ts << line << "\n";
        peplF.close();
      } else
        std::cerr << "Failed to open listing for writing: " << *osListOut
                  << std::endl;
    } catch (std::exception &e) {
      std::cerr << "Failed to generate listing due to internal bug.\n";
    }
  }
  {
    QFileInfo pepo(QString::fromStdString(userIn));
    QString pepoFName = pepo.path() + "/" + pepo.completeBaseName() + ".pepo";
    auto userBytes = pas::ops::pepp::toBytes<isa::Pep10>(*userRoot);
    auto userBytesStr = pas::ops::pepp::bytesToObject(userBytes);
    QFile pepoF(pepoFName);
    if (pepoF.open(QIODevice::WriteOnly | QIODevice::Truncate |
                   QIODevice::Text)) {
      QTextStream(&pepoF) << userBytesStr << "\n";
    } else
      std::cerr << "Failed to open object code for writing: "
                << pepoFName.toStdString() << std::endl;
  }

  return emit finished(0);
}
