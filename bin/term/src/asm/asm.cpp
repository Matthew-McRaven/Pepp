#include "asm.hpp"
#include "../shared.hpp"
#include "builtins/figure.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include <iostream>

AsmTask::AsmTask(int ed, std::string userFname, std::string out,
                 QObject *parent)
    : Task(parent), ed(ed), userIn(userFname), pepoOut(out) {}

void AsmTask::setOsFname(std::string fname) { osIn = fname; }

void AsmTask::setOsListingFname(std::string fname) { osListOut = fname; }

void AsmTask::emitElfTo(std::string fname) { elfOut = fname; }

void AsmTask::run() {
  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);
  auto macroRegistry = detail::registry(book, {});

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
  detail::AsmHelper helper(macroRegistry, osContents);
  helper.setUserText(userContents);
  auto result = helper.assemble();
  if (!result) {
    std::cerr << "Assembly failed, check error log" << std::endl;
    QFileInfo err(QString::fromStdString(userIn));
    QString errFName = err.path() + "/" + err.completeBaseName() + ".err.txt";
    QFile errF(errFName);
    if (errF.open(QIODevice::WriteOnly | QIODevice::Truncate |
                  QIODevice::Text)) {
      auto ts = QTextStream(&errF);
      for (auto &error : helper.errors())
        ts << error;
    } else {
      std::cerr << "Failed to open error log for writing: "
                << errFName.toStdString() << std::endl;
    }
    return emit finished(1);
  }
  // Assembly succeded!
  if (elfOut.has_value()) {
    auto elf = helper.elf();
    elf->save(elfOut.value());
  }

  try {
    auto lines = helper.listing(false);
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
      auto lines = helper.listing(true);
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
    auto userBytes = helper.bytes(false);
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
