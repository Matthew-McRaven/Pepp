#include "./macro.hpp"
#include "builtins/book.hpp"
#include "builtins/registry.hpp"
#include <iostream>

GetMacroTask::GetMacroTask(int ed, std::string name, QObject *parent)
    : Task(parent), ed(ed), name(name) {}

void GetMacroTask::run() {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    emit finished(1);
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);

  auto macro = book->findMacro(QString::fromStdString(name));
  if (macro.isNull())
    return emit finished(1);

  auto body = macro->body();
  std::cout << body.toStdString() << std::endl;

  emit finished(0);
}
